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

const int MAX_RECEIVE_LENGTH = 1024 * 1024 * 10; // 10MB

int write_n(int sockfd, const void *buf, int length) {
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

int read_n(int sockfd, void *buf, int length) {
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

void transmit(const Option &option) {
  struct sockaddr_in addr = resolve_or_die(option.host.c_str(), option.port);

  printf("connecting to %s:%d\n", inet_ntoa(addr.sin_addr), option.port);

  int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  assert(sockfd >= 0);

  int ret = connect(sockfd, (sockaddr *)&addr, sizeof(addr));
  if (ret) {
    perror("connet");
    printf("Unable to connect %s\n", option.host.c_str());
    close(sockfd);
    return;
  }

  printf("connected\n");

  auto start = std::chrono::high_resolution_clock::now();

  struct SessionMessage session_message = {0, 0};
  session_message.number = ntohl(option.number);
  session_message.length = ntohl(option.length);

  if (write_n(sockfd, &session_message, sizeof(session_message)) !=
      sizeof(session_message)) {
    perror("write session message");
    exit(1);
  }

  const int total_len = int(sizeof(int32_t) + option.length);

  PayloadMessage *payload = (PayloadMessage *)(malloc(total_len));

  assert(payload);
  payload->length = htonl(option.length);
  for (int i = 0; i < option.length; ++i) {
    payload->data[i] = "0123456789ABCDEF"[i % 16];
  }

  double total_mb = 1.0 * option.length * option.number / 1024 / 1024;
  printf("%.3f MiB in total\n", total_mb);

  for (int i = 0; i < option.number; ++i) {
    int nw = write_n(sockfd, payload, total_len);
    assert(nw == total_len);

    int ack = 0;
    int nr = read_n(sockfd, &ack, sizeof(ack));
    assert(nr == sizeof(ack));
    ack = ntohl(ack);
    assert(ack == option.length);
  }

  free(payload);
  close(sockfd);

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elasped_seconds = (end - start);
  printf("%.3f seconds\n%.3f Mib/s\n", elasped_seconds.count(),
         total_mb / elasped_seconds.count());
}

void receive(const Option &option) {
  int sockfd = accept_or_die(option.port);

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
  Option option;

  if (parse_command_line(argc, argv, &option)) {
    if (option.transmit) {
      transmit(option);
    } else if (option.receive) {
      receive(option);
    } else {
      assert(0);
    }
  }

  return 0;
}
