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

#include "ttcp_libev_test.h"

uint16_t PORT = 5002;

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
}

static void server_cb(EV_P_ ev_io *w, int revents) {
  printf("server fd become readable\n");

  EvClientPtr ev_client = EvClientPtr();

  EvServer *ev_server = (EvServer *)w;
  assert(ev_server->fd >= 0);

  ev_client->fd = accept(ev_server->fd, NULL, NULL);
  assert(ev_client->fd >= 0);

  int rtn = setnonblock(ev_client->fd);
  assert(rtn != -1);

  ev_io_init(&ev_client->io, client_cb, ev_client->fd, EV_READ);
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