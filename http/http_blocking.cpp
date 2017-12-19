#include <arpa/inet.h>  // uint16_t
#include <assert.h>     // assert
#include <errno.h>      // perror
#include <netinet/in.h> // sockaddr_in
#include <signal.h>     // signal
#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <strings.h>    // bzero
#include <sys/socket.h> // socket
#include <sys/types.h> // some historical (BSD) implementations required this header file
#include <unistd.h> // close

#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>

void raw_print(char c) {
  const char *buf = NULL;
  switch (c) {
  case '\r':
    buf = "\\r";
    break;
  case '\n':
    buf = "\\n\n";
    break;
  }
  if (buf) {
    fwrite(buf, 1, strlen(buf), stdout);
    return;
  }

  fwrite(&c, 1, sizeof(c), stdout);
}

void accept_request(int client_fd) {
  LOG_INFO << "new client_fd = " << client_fd;

  while (true) {
    char c;
    ssize_t nr = read(client_fd, &c, 1);
    if (nr < 0) {
      if (errno == EINTR) {
        continue;
      } else {
        LOG_ERROR << "read" << strerror(errno);
        break;
      }
    } else if (nr == 0) {
      LOG_INFO << "eof client_fd = " << client_fd;
      break;
    } else {
      raw_print(c);
      // FIXME: move to buffer
    }
  }

  if (::shutdown(client_fd, SHUT_WR) < 0) {
    LOG_ERROR << "shutdown" << strerror(errno);
  }

  char buf[1024];
  while (read(client_fd, buf, sizeof(buf)) > 0) {
    // do nothing
  }

  LOG_INFO << "done client_fd = " << client_fd;
}

int get_listen_fd_or_die(uint16_t port) {
  int listenfd = socket(PF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    LOG_FATAL << "socket" << strerror(errno);
  }

  // SO_REUSEADDR
  int yes = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
    LOG_FATAL << "setsockopt" << strerror(errno);
  }

  // SIGPIPE
  signal(SIGPIPE, SIG_IGN);

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(listenfd, reinterpret_cast<struct sockaddr *>(&addr),
           sizeof(addr))) {
    LOG_FATAL << "bind" << strerror(errno);
  }

  if (listen(listenfd, 5)) {
    LOG_FATAL << "listen" << strerror(errno);
  }

  return listenfd;
}

int main(int argc, char *argv[]) {
  muduo::TimeZone beijing(8 * 3600, "CST");
  muduo::Logger::setTimeZone(beijing);

  if (argc < 2) {
    printf("Usage:\n%s port\n", argv[0]);
    exit(1);
  }

  uint16_t port = atoi(argv[1]);

  int listen_fd = get_listen_fd_or_die(port);

  LOG_INFO << "listening on port = " << port;

  while (1) {
    struct sockaddr_in peer_addr;
    bzero(&peer_addr, sizeof(peer_addr));
    socklen_t addrlen = 0;

    int client_fd = accept(
        listen_fd, reinterpret_cast<struct sockaddr *>(&peer_addr), &addrlen);
    if (client_fd < 0) {
      LOG_FATAL << "accept" << strerror(errno);
    }

    std::thread thr(accept_request, client_fd);
    thr.detach();
  }

  close(listen_fd);

  return 0;
}