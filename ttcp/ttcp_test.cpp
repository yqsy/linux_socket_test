// #define NDEBUG
#include <arpa/inet.h>
#include <cassert>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <chrono>
#include <iostream>
#include <string>

#include <boost/program_options.hpp>

#include "common.h"
#include "ttcp_test.h"

namespace po = boost::program_options;

uint16_t PORT;
int BUFFER_LENGTH;
int BUFFER_NUMBER;
bool TRANSMIT;
std::string TO_HOST;

const int MAX_RECEIVE_LENGTH = 1024 * 1024 * 10; // 10MB

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

static sockaddr_in resolve_or_die(const char *host, uint16_t port) {
  struct hostent *he = gethostbyname(host);
  if (!he) {
    perror("gethostbyname");
    exit(1);
  }

  assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = *(in_addr *)he->h_addr;
  return addr;
}

void transmit() {
  struct sockaddr_in addr = resolve_or_die(TO_HOST.c_str(), PORT);

  printf("connecting to %s:%d\n", inet_ntoa(addr.sin_addr), PORT);

  int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  assert(sockfd >= 0);

  int ret = connect(sockfd, (sockaddr *)&addr, sizeof(addr));
  if (ret) {
    perror("connet");
    printf("Unable to connect %s\n", TO_HOST.c_str());
    close(sockfd);
    return;
  }

  printf("connected\n");

  auto start = std::chrono::high_resolution_clock::now();

  struct SessionMessage session_message = {0, 0};
  session_message.number = ntohl(BUFFER_NUMBER);
  session_message.length = ntohl(BUFFER_LENGTH);

  if (write_n(sockfd, &session_message, sizeof(session_message)) !=
      sizeof(session_message)) {
    perror("write session message");
    exit(1);
  }

  const int total_len = int(sizeof(int32_t) + BUFFER_LENGTH);

  PayloadMessage *payload = (PayloadMessage *)(malloc(total_len));

  assert(payload);
  payload->length = htonl(BUFFER_LENGTH);
  for (int i = 0; i < BUFFER_LENGTH; ++i) {
    payload->data[i] = "0123456789ABCDEF"[i % 16];
  }

  double total_mb = 1.0 * BUFFER_LENGTH * BUFFER_NUMBER / 1024 / 1024;
  printf("%.3f MiB in total\n", total_mb);

  for (int i = 0; i < BUFFER_NUMBER; ++i) {
    int nw = write_n(sockfd, payload, total_len);
    assert(nw == total_len);

    int ack = 0;
    int nr = read_n(sockfd, &ack, sizeof(ack));
    assert(nr == sizeof(ack));
    ack = ntohl(ack);
    assert(ack == BUFFER_LENGTH);
  }

  free(payload);
  close(sockfd);

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elasped_seconds = (end - start);
  printf("%.3f seconds\n%.3f Mib/s\n", elasped_seconds.count(),
         total_mb / elasped_seconds.count());
}

void receive() {
  int sockfd = accept_or_die(PORT);

  struct SessionMessage session_message = {0, 0};
  if (read_n(sockfd, &session_message, sizeof(session_message)) !=
      sizeof(session_message)) {
    perror("read SessionMessage");
    exit(1);
  }

  session_message.number = ntohl(session_message.number);
  session_message.length = ntohl(session_message.length);
  printf("receive number = %d\nreceived length = %d\n", session_message.number,
         session_message.length);

  if (session_message.length > MAX_RECEIVE_LENGTH) {
    perror("read error length");
    exit(1);
  }

  const int total_len = int(sizeof(int32_t) + session_message.length);
  PayloadMessage *payload = (PayloadMessage *)malloc(total_len);
  assert(payload);

  for (int i = 0; i < session_message.number; ++i) {
    payload->length = 0;
    if (read_n(sockfd, &payload->length, sizeof(payload->length)) !=
        sizeof(payload->length)) {
      perror("read length");
      exit(1);
    }

    payload->length = ntohl(payload->length);
    assert(payload->length == session_message.length);

    if (read_n(sockfd, payload->data, payload->length) != payload->length) {
      perror("read payload data");
      exit(1);
    }
    int32_t ack = htonl(payload->length);
    if (write_n(sockfd, &ack, sizeof(ack)) != sizeof(ack)) {
      perror("write ack");
      exit(1);
    }
  }
  free(payload);
  close(sockfd);
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Help")(
      "port,p", po::value<uint16_t>(&PORT)->default_value(5001), "TCP port")(
      "length,l", po::value<int>(&BUFFER_LENGTH)->default_value(65536),
      "Buffer length")(
      "number,n", po::value<int>(&BUFFER_NUMBER)->default_value(8192),
      "Number of buffers")("trans,t", "Transmit")("recv,r", "Receive")(
      "host", po::value<std::string>(&TO_HOST)->default_value("127.0.0.1"),
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
    transmit();
  } else if (vm.count("recv")) {
    printf("port = %d\n", PORT);
    printf("accepting...\n");
    receive();
  }

  return 0;
}
