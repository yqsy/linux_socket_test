#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ev.h>

#include <cassert>
#include <memory>
#include <vector>

#include "common.h"
#include "ttcp_libev_test.h"

uint16_t PORT = 5002;

const int MAX_RECEIVE_LENGTH = 1024 * 1024 * 10; // 10MB

int setnonblock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

int get_listen_fd(uint16_t port) {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(fd >= 0);

  int rtn = setnonblock(fd);
  assert(rtn != -1);

  int yes = 1;
  rtn = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  assert(rtn == 0);

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  rtn = bind(fd, (sockaddr *)(&addr), sizeof(addr));
  assert(rtn == 0);

  rtn = listen(fd, 5);
  assert(rtn == 0);

  return fd;
}

static void client_cb(EV_P_ ev_io *w, int revents) {
  EvClient *ev_client = (EvClient *)w;

  struct SessionMessage session_message = {0, 0};
  int rb = read_n(ev_client->fd, &session_message, sizeof(session_message));
  assert(rb = sizeof(session_message));

  session_message.number = ntohl(session_message.number);
  session_message.length = ntohl(session_message.length);
  printf("receive number = %d\nreceived length = %d\n", session_message.number,
         session_message.length);

  assert(session_message.length <= MAX_RECEIVE_LENGTH);

  const int total_len = int(sizeof(int32_t) + session_message.length);
  PayloadMessage *payload = (PayloadMessage *)malloc(total_len);
  assert(payload);

  for (int i = 0; i < session_message.number; ++i) {
    payload->length = 0;

    rb = read_n(ev_client->fd, &payload->length, sizeof(payload->length));
    assert(rb == sizeof(payload->length));

    payload->length = ntohl(payload->length);
    assert(payload->length == session_message.length);

    rb = read_n(ev_client->fd, payload->data, payload->length);
    assert(rb == payload->length);

    int32_t ack = htonl(payload->length);

    int wb = write_n(ev_client->fd, &ack, sizeof(ack));
    assert(wb == sizeof(ack));
  }

  free(payload);
  close(ev_client->fd);

  // FIXME: O(n)
  auto &ev_clients = ev_client->ev_server->ev_clients;
  for (auto iter = ev_clients.begin(); iter != ev_clients.end(); ++iter) {
    if (&**iter == ev_client) {
      ev_clients.erase(iter);
    }
  }
}

static void server_cb(EV_P_ ev_io *w, int revents) {
  printf("server fd become readable\n");

  EvClientPtr ev_client = std::make_shared<EvClient>();

  EvServer *ev_server = (EvServer *)w;
  assert(ev_server->fd >= 0);

  ev_client->fd = accept(ev_server->fd, NULL, NULL);
  assert(ev_client->fd >= 0);

  int rtn = setnonblock(ev_client->fd);
  assert(rtn != -1);

  ev_io_init(&ev_client->io, client_cb, ev_client->fd, EV_READ);
  ev_io_start(EV_A_ & ev_client->io);
}

int main() {
  EV_P = ev_default_loop(0);

  EvServer server;

  server.fd = get_listen_fd(PORT);
  ev_io_init(&server.io, server_cb, server.fd, EV_READ);
  ev_io_start(EV_A_ & server.io);

  printf("listening on port = %d, looping\n", PORT);

  ev_loop(EV_A_ 0);

  close(server.fd);
  return 0;
}