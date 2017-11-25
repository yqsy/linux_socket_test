// #define NDEBUG

#include <cassert>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <boost/program_options.hpp>
#include <iostream>
#include <string>

static int accept_or_die(uint16_t port) {
  int listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(listenfd >= 0);

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

  if (bind(listenfd, (sockaddr *)(&addr), sizeof(addr))) {
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
  int sockfd = accept(listenfd, (sockaddr *)(&peer_addr), &addrlen);

  if (sockfd < 0) {
    perror("accept");
    exit(1);
  }
  close(listenfd);
  return sockfd;
}

static int write_n(int sockfd, const void *buf, int length) {
  int written = 0;
  while (written < length) {
    ssize_t nw = write(sockfd, (const char *)buf + written, length - written);
    if (nw > 0) {
      written += int(nw);
    } else if (nw == 0) {
      break; // EOF
    } else if (errno != EINTR) {
      perror("write");
      break;
    }
  }
  return written;
}

static int read_n(int sockfd, void *buf, int length) {
  int nread = 0;
  while (nread < length) {
    ssize_t nr = read(sockfd, (char *)buf + nread, length - nread);
    if (nr > 0) {
      nread += (int)nr;
    } else if (nr == 0) {
      break; // EOF
    } else if (errno != EINTR) {
      perror("read");
      break;
    }
  }
  return nread;
}

namespace po = boost::program_options;

uint16_t PORT;
int BUFFER_LENGTH;
int BUFFER_NUMBER;
bool TRANSMIT;
std::string TO_HOST;

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Help")(
      "port,p", po::value<uint16_t>(&PORT)->default_value(5001), "TCP port")(
      "length,l", po::value<int>(&BUFFER_LENGTH)->default_value(65536),
      "Buffer length")(
      "number,n", po::value<int>(&BUFFER_NUMBER)->default_value(8192),
      "Number of buffers")("trans,t", "Transmit")("recv,r", "Receive")(
      "host,h", po::value<std::string>(&TO_HOST)->default_value("127.0.0.1"),
      "Transmit to host");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  if (vm.count("trans") && vm.count("recv")) {
    printf("-t -r must be specified.\n");
    return 0;
  }

  if (vm.count("trans")) {
    printf("host = %s, port = %d\n", TO_HOST.c_str(), PORT);
    printf("buffer length = %d\n", BUFFER_LENGTH);
    printf("number of buffers = %d\n", BUFFER_NUMBER);

  } else if (vm.count("recv")) {
    printf("port = %d\n", PORT);
    printf("accepting...\n");
  }

  return 0;
}
