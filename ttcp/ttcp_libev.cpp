#include <fcntl.h>
#include <unistd.h>

#include <ev.h>
#include <memory>
#include <vector>

#include <circular_buffer/circular_buffer.h>
#include <ttcp/common.h>

struct EvClient;
struct EvServer;

typedef std::shared_ptr<EvClient> EvClientPtr;

enum RecvState { k_except_frame_size, k_except_frame };

struct EvClient {
  ev_io io;
  int fd;
  RecvState state;
  int read_number;
  EvServer *server_;
  CircularBuffer input_buffer;
  SessionMessage smsg;

  EvClient(EvServer *server)
      : fd(0), state(k_except_frame_size), read_number(0), server_(server) {
    smsg.number = 0;
    smsg.length = 0;
  }
};

struct EvServer {
  ev_io io;
  int fd;
  std::vector<EvClientPtr> ev_clients;

  EvServer() : fd(0) {}

  void delete_client(const EvClient *ev_client) {
    for (auto iter = ev_clients.begin(); iter != ev_clients.end();) {
      if (&**iter == ev_client) {
        iter = ev_clients.erase(iter);
        break;
      } else {
        ++iter;
      }
    }
  }
};

int setnonblock(int fd) {
  int flags = fcntl(fd, F_GETFL);
  flags |= O_NONBLOCK;
  return fcntl(fd, F_SETFL, flags);
}

int get_listenfd_or_die(uint16_t port) {
  int listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    perror("socket");
    exit(1);
  }

  if (setnonblock(listenfd)) {
    perror("setnonblock");
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

  return listenfd;
}

void client_read_cb(struct ev_loop *loop, ev_io *w, int revents) {
  auto ev_client = reinterpret_cast<EvClient *>(w);
  auto &smsg = ev_client->smsg;
  auto &input_buffer = ev_client->input_buffer;

  input_buffer.readfd(ev_client->fd);

  for (;;) {
    if (ev_client->state == k_except_frame_size) {
      if (input_buffer.readable_bytes() >= sizeof(smsg)) {
        smsg.number = input_buffer.read_int32();
        smsg.length = input_buffer.read_int32();

        printf("receive number = %d\nreceived length = %d\n", smsg.number,
               smsg.length);

        if (smsg.number <= 0 || smsg.length <= 0 ||
            smsg.length > MAX_BUFFER_LENGTH) {
          perror("read error length");
          exit(1);
        }

        ev_client->state = k_except_frame;
      } else {
        break;
      }
    } else if (ev_client->state == k_except_frame) {
      const unsigned int total_len =
          static_cast<int>(sizeof(int32_t)) + smsg.length;
      if (input_buffer.readable_bytes() >= total_len) {
        int32_t payload_len = input_buffer.read_int32();
        if (payload_len != smsg.length) {
          printf("payload_len read error = %d\n", payload_len);
          exit(1);
        }
        input_buffer.retrieve(smsg.length);

        int32_t ack = htonl(payload_len);
        // FIXME: output buffer
        ::write(ev_client->fd, &ack, sizeof(ack));

        ++ev_client->read_number;
        if (ev_client->read_number >= smsg.number) {
          // FIXME: shutdown
          close(ev_client->fd);
          ev_io_stop(loop, &ev_client->io);
          ev_client->server_->delete_client(ev_client);
        }

      } else {
        break;
      }
    }
  }
}

void server_read_cb(struct ev_loop *loop, ev_io *w, int revents) {
  auto ev_server = reinterpret_cast<EvServer *>(w);

  auto ev_client = std::make_shared<EvClient>(ev_server);

  ev_client->fd = ::accept(ev_server->fd, NULL, NULL);

  if (ev_client->fd < 0) {
    perror("accept");
    exit(1);
  }

  if (setnonblock(ev_client->fd)) {
    perror("setnonblock");
    exit(1);
  }

  ev_server->ev_clients.push_back(ev_client);
  ev_io_init(&ev_client->io, client_read_cb, ev_client->fd, EV_READ);
  ev_io_start(loop, &ev_client->io);
}

void sigint_cb(struct ev_loop *loop, ev_signal *w, int revents) {
  printf("exit\n");
  ev_break(loop, EVBREAK_ALL);
}

void server_receive(const Options &opt) {
  struct ev_loop *loop = ev_default_loop(0);

  // SIGINT
  ev_signal signal_watcher;
  ev_signal_init(&signal_watcher, sigint_cb, SIGINT);
  ev_signal_start(loop, &signal_watcher);

  // Server read
  EvServer server;
  server.fd = get_listenfd_or_die(opt.listen_port);
  ev_io_init(&server.io, server_read_cb, server.fd, EV_READ);
  ev_io_start(loop, &server.io);

  printf("listening on port = %d, looping\n", opt.listen_port);
  ev_loop(loop, 0);
  close(server.fd);
}

void client_send(const Options &opt) {}

int main(int argc, char *argv[]) {
  Options options;

  parse_commandline(argc, argv, &options);

  if (options.command == k_server) {
    server_receive(options);
  } else if (options.command == k_client) {
    client_send(options);
  }

  return 0;
}