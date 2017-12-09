
#include <arpa/inet.h>  // uint16_t
#include <assert.h>     // assert
#include <errno.h>      // perror
#include <netinet/in.h> // sockaddr_in
#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <strings.h>    // bzero
#include <sys/socket.h> // socket
#include <sys/types.h> // some historical (BSD) implementations required this header file
#include <unistd.h> // close

#include <ttcp/common.h>

#include <chrono>

int accept_or_die(uint16_t port) {
  int listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    perror("socket");
    exit(1);
  }

  int yes = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
    perror("setsockopt");
    exit(1);
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(listenfd, reinterpret_cast<struct sockaddr *>(&addr),
           sizeof(addr))) {
    perror("bind");
    exit(1);
  }

  if (listen(listenfd, 5)) {
    perror("listen");
    exit(1);
  }

  struct sockaddr_in peer_addr;
  bzero(&peer_addr, sizeof(peer_addr));
  socklen_t addrlen = 0;
  int sockfd = ::accept(
      listenfd, reinterpret_cast<struct sockaddr *>(&peer_addr), &addrlen);

  if (sockfd < 0) {
    perror("accept");
    exit(1);
  }

  ::close(listenfd);
  return sockfd;
}

int write_n(int sockfd, const void *buf, int length) {
  int written = 0;
  while (written < length) {
    ssize_t nw = ::write(sockfd, static_cast<const char *>(buf) + written,
                         length - written);
    if (nw > 0) {
      written += static_cast<int>(nw);
    } else if (nw == 0) {
      break; // EOF
    } else if (errno != EINTR) {
      perror("write");
      break;
    }
  }
  return written;
}

int read_n(int sockfd, void *buf, int length) {
  int nread = 0;
  while (nread < length) {
    ssize_t nr =
        ::read(sockfd, static_cast<char *>(buf) + nread, length - nread);
    if (nr > 0) {
      nread += static_cast<int>(nr);
    } else if (nr == 0) {
      break; // EOF
    } else if (errno != EINTR) {
      perror("read");
      break;
    }
  }
  return nread;
}

void server_receive(const Options &opt) {
  printf("listening on :%d\n", opt.listen_port);

  int sockfd = accept_or_die(opt.listen_port);

  struct SessionMessage smsg = {0, 0};
  if (read_n(sockfd, &smsg, sizeof(smsg)) != sizeof(smsg)) {
    perror("read SessionMessage");
    exit(1);
  }

  smsg.number = ntohl(smsg.number);
  smsg.length = ntohl(smsg.length);

  printf("receive number = %d\nreceive length = %d\n", smsg.number,
         smsg.length);

  if (smsg.length > MAX_BUFFER_LENGTH) {
    printf("receive length to max\n");
    exit(1);
  }

  const int total_len = static_cast<int>(sizeof(int32_t) + smsg.length);
  PayloadMessage *payload = static_cast<PayloadMessage *>(::malloc(total_len));

  for (int i = 0; i < smsg.number; ++i) {

    if (read_n(sockfd, &payload->length, total_len) != total_len) {
      perror("read length");
      exit(1);
    }

    payload->length = ntohl(payload->length);
    assert(payload->length == smsg.length);
    int32_t ack = htonl(payload->length);
    if (write_n(sockfd, &ack, sizeof(ack)) != sizeof(ack)) {
      perror("write ack");
      exit(1);
    }
  }

  ::free(payload);
  ::close(sockfd);
}

void client_send(const Options &opt) {
  struct sockaddr_in addr =
      resolve_or_die(opt.server_host.c_str(), opt.server_port);

  printf("connecting to %s:%d\n", inet_ntoa(addr.sin_addr), opt.server_port);

  int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    exit(1);
  }

  int ret = ::connect(sockfd, reinterpret_cast<struct sockaddr *>(&addr),
                      sizeof(addr));
  if (ret) {
    perror("connect");
    ::close(sockfd);
    exit(1);
  }

  printf("connected\n");

  auto start = std::chrono::high_resolution_clock::now();

  struct SessionMessage smsg = {0, 0};
  smsg.number = htonl(opt.number);
  smsg.length = htonl(opt.length);
  if (write_n(sockfd, &smsg, sizeof(smsg)) != sizeof(smsg)) {
    perror("write sessionmessage");
    exit(1);
  }

  const int total_len = static_cast<int>(sizeof(int32_t) + opt.length);
  PayloadMessage *payload = static_cast<PayloadMessage *>(::malloc(total_len));
  assert(payload);
  payload->length = htonl(opt.length);

  for (int i = 0; i < opt.length; ++i) {
    payload->data[i] = "0123456789ABCDEF"[i % 16];
  }

  double total_mb = 1.0 * opt.number * opt.length / 1024 / 1024;
  printf("%.3f Mib in total\n", total_mb);

  for (int i = 0; i < opt.number; ++i) {
    int nw = write_n(sockfd, payload, total_len);
    assert(nw == total_len);

    int ack = 0;
    int nr = read_n(sockfd, &ack, sizeof(ack));
    assert(nr == sizeof(ack));
    ack = ntohl(ack);
    assert(ack == opt.length);
  }

  ::free(payload);
  ::close(sockfd);

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elasped_seconds = (end - start);
  printf("%.3f seconds\n%.3f Mib/s\n", elasped_seconds.count(),
         total_mb / elasped_seconds.count());
}

int main(int argc, char *argv[]) {

  // int sockfd = accept_or_die(PORT);
  Options options;

  parse_commandline(argc, argv, &options);

  if (options.command == k_server) {
    server_receive(options);
  } else if (options.command == k_client) {
    client_send(options);
  }

  return 0;
}