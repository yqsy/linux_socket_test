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

static void client_state_cb(struct ev_loop *loop, ev_io *w, int revents) {
  EvClient *ev_client = (EvClient *)w;
  auto &session_message = ev_client->session_message;
  ev_client->interrupt_count++;

  size_t rb = 0;

  for (;;) {
    if (ev_client->recv_state == kExpectFrameSize) {
      if (gkrbs(ev_client->fd) >= sizeof(SessionMessage)) {
        rb = read(ev_client->fd, (char *)&session_message,
                  sizeof(session_message));
        assert(rb == sizeof(session_message));
        session_message.number = ntohl(session_message.number);
        session_message.length = ntohl(session_message.length);
        printf("receive number = %d\nreceived length = %d\n",
               session_message.number, session_message.length);

        assert(session_message.number > 0 && session_message.length > 0);
        if (session_message.length > MAX_RECEIVE_LENGTH) {
          perror("read error length");
          exit(1);
        }
        assert(ev_client->buffer == NULL);

        const int total_len = int(sizeof(int32_t) + session_message.length);
        ev_client->buffer = (char *)malloc(total_len);
        assert(ev_client->buffer);
        ev_client->ack = htonl(session_message.length);

        ev_client->recv_state = kExceptFrame;
      } else {
        break;
      }
    } else if (ev_client->recv_state == kExceptFrame) {
      // TODO: user level buffer
      const size_t total_len = sizeof(int32_t) + size_t(session_message.length);
      if (gkrbs(ev_client->fd) >= total_len) {
        rb = read(ev_client->fd, ev_client->buffer, total_len);
        assert(rb == total_len);

        int *header4_bytes = (int *)ev_client->buffer;
        assert(ntohl(*header4_bytes) == size_t(session_message.length));

        int wb = write(ev_client->fd, &ev_client->ack, sizeof(ev_client->ack));
        assert(wb == sizeof(ev_client->ack));

        ++ev_client->count;
        if (ev_client->count >= session_message.number) {
          printf("delete client = %d, interrupt_count = %d\n", ev_client->fd,
                 ev_client->interrupt_count);
          ev_io_stop(loop, &ev_client->io);
          close(ev_client->fd);
          free(ev_client->buffer);
          // FIXME: O(n)
          auto &ev_clients = ev_client->ev_server->ev_clients;

          for (auto iter = ev_clients.begin(); iter != ev_clients.end();) {
            if (&**iter == ev_client) {
              iter = ev_clients.erase(iter);
            } else {
              ++iter;
            }
          }
        }
      } else {
        break;
      }
    }
  }
}

static void server_cb(struct ev_loop *loop, ev_io *w, int revents) {
  printf("server fd become readable\n");

  EvClientPtr ev_client = std::make_shared<EvClient>();
  ev_client->session_message.number = 0;
  ev_client->session_message.length = 0;
  ev_client->recv_state = kExpectFrameSize;

  EvServer *ev_server = (EvServer *)w;
  assert(ev_server->fd >= 0);

  ev_client->ev_server = ev_server;

  ev_server->ev_clients.push_back(ev_client);

  ev_client->fd = accept(ev_server->fd, NULL, NULL);
  assert(ev_client->fd >= 0);

  int rtn = setnonblock(ev_client->fd);
  assert(rtn != -1);

  ev_io_init(&ev_client->io, client_state_cb, ev_client->fd, EV_READ);
  ev_io_start(loop, &ev_client->io);
}

int main(int argc, char *argv[]) {

  Option option;
  if (parse_command_line(argc, argv, &option)) {
    if (option.receive) {

      struct ev_loop *loop = ev_default_loop(0);

      EvServer server;

      server.fd = get_listen_fd(option.port);
      ev_io_init(&server.io, server_cb, server.fd, EV_READ);
      ev_io_start(loop, &server.io);

      printf("listening on port = %d, looping\n", option.port);

      ev_loop(loop, 0);

      close(server.fd);
    } else {
      assert(0);
    }
  }

  return 0;
}