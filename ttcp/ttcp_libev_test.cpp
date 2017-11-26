#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
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

// get_kernel_recv_buffer_size
static size_t gkrbs(int fd) {
  int krbs = 0;
  ioctl(fd, FIONREAD, &krbs);
  return krbs;
}

static void client_cb(EV_P_ ev_io *w, int revents) {
  EvClient *ev_client = (EvClient *)w;
  auto &session_message = ev_client->session_message;

  size_t rb = 0;

  while (gkrbs(ev_client->fd) >= sizeof(int32_t)) {
    if (session_message.number == 0 && session_message.length == 0) {
      if (gkrbs(ev_client->fd) >= sizeof(SessionMessage)) {
        rb = read(ev_client->fd, (char *)&session_message,
                  sizeof(session_message));
        assert(rb == sizeof(session_message));
        session_message.number = ntohl(session_message.number);
        session_message.length = ntohl(session_message.length);

        assert(ev_client->buffer == NULL);
        const int total_len = int(sizeof(int32_t) + session_message.length);
        ev_client->buffer = (char *)malloc(total_len);
        bzero(ev_client->buffer, total_len);
        ev_client->ack = session_message.length;
      } else {
        break;
      }
    } else {
      const size_t total_len = sizeof(int32_t) + size_t(session_message.length);
      int32_t length = 0;
      rb = recv(ev_client->fd, (char *)&length, sizeof(length), MSG_PEEK);
      assert(rb == sizeof(int32_t));

      length = ntohl(length);
      assert(length == session_message.length);

      if (gkrbs(ev_client->fd) >= total_len) {
        assert(ev_client->buffer != NULL);
        rb = read(ev_client->fd, ev_client->buffer, total_len);
        assert(rb == total_len);

        // FIXME: send return?
        if (write(ev_client->fd, &ev_client->ack, sizeof(ev_client->ack)) !=
            sizeof(ev_client->ack)) {
          perror("write");
          exit(0);
        }

        ++ev_client->count;
        if (ev_client->count >= session_message.number) {
          close(ev_client->fd);
          free(ev_client->buffer);
          // FIXME: O(n)
          auto &ev_clients = ev_client->ev_server->ev_clients;
          for (auto iter = ev_clients.begin(); iter != ev_clients.end();
               ++iter) {
            if (&**iter == ev_client) {
              ev_clients.erase(iter);
            }
          }
        }

      } else {
        break;
      }
    }
  }
}

static void server_cb(EV_P_ ev_io *w, int revents) {
  printf("server fd become readable\n");

  EvClientPtr ev_client = std::make_shared<EvClient>();
  ev_client->session_message.number = 0;
  ev_client->session_message.length = 0;

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