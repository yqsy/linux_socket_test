
#include <arpa/inet.h>  // uint16_t
#include <errno.h>      // perror
#include <netinet/in.h> // sockaddr_in
#include <stdio.h>      // perror
#include <stdlib.h>     // exit
#include <strings.h>    // bzero
#include <sys/socket.h> // socket
#include <sys/types.h> // some historical (BSD) implementations required this header file
#include <unistd.h> // close

#include <ttcp/common.h>

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

uint16_t PORT = 5001;

int main(int argc, char *argv[]) {

  // int sockfd = accept_or_die(PORT);
  Options options;

  parse_commandline(argc, argv, &options);

  return 0;
}