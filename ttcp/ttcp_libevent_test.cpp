
#include <assert.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>


#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include <iostream>

#include "common.h"
#include "ttcp_libevent_test.h"

const int MAX_LINE = 16384; // 1024 * 16

const int MAX_RECEIVE_LENGTH = 1024 * 1024 * 10; // 10MB

#define assert__(x) for (; !(x); assert(x))

int get_listen_fd(uint16_t port) {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(fd >= 0);

  int yes = 1;
  int rtn = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
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

void signal_cb(evutil_socket_t sig, short events, void *user_data) {

  event_base *base = (event_base *)user_data;
  timeval delay = {2, 0};

  printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");
  event_base_loopexit(base, &delay);
}

void read_cb(bufferevent *bev, void *ctx) {
  evbuffer *input = bufferevent_get_input(bev);
  evbuffer *output = bufferevent_get_output(bev);
  assert(input);
  assert(output);

  auto event_client = (EventClient *)ctx;

  // std::cout << "read length: " << evbuffer_get_length(input) << "\n";
  // printf("read length: %u\n", evbuffer_get_length(input));
  auto &session_message = event_client->session_message;
  for (;;) {
    if (event_client->recv_state == kExpectFrameSize) {
      if (evbuffer_get_length(input) >= sizeof(SessionMessage)) {
        size_t rb =
            evbuffer_remove(input, &session_message, sizeof(SessionMessage));
        assert(rb == sizeof(SessionMessage));
        session_message.number = ntohl(session_message.number);
        session_message.length = ntohl(session_message.length);
        printf("receive number = %d\nreceived length = %d\n",
               session_message.number, session_message.length);
        assert(session_message.number > 0 && session_message.length > 0);
        if (session_message.length > MAX_RECEIVE_LENGTH) {
          perror("read error length");
          exit(1);
        }

        assert(event_client->buffer == NULL);

        const int total_len = int(sizeof(int32_t) + session_message.length);
        event_client->buffer = (char *)malloc(total_len);
        assert(event_client->buffer);

        event_client->ack = htonl(session_message.length);

        event_client->recv_state = kExceptFrame;
      } else {
        break;
      }

    } else if (event_client->recv_state == kExceptFrame) {
      const size_t total_len = sizeof(int32_t) + size_t(session_message.length);
      if (evbuffer_get_length(input) >= total_len) {
        size_t rb = evbuffer_remove(input, event_client->buffer, total_len);

        assert__(rb == total_len) { printf("%lu != %lu\n", rb, total_len); }

        int *header4_bytes = (int *)event_client->buffer;
        assert(ntohl(*header4_bytes) == size_t(session_message.length));

        int rtn =
            evbuffer_add(output, &event_client->ack, sizeof(event_client->ack));
        assert(rtn == 0);

        ++event_client->count;
        if (event_client->count >= session_message.number) {
          // FIXME: FIN? half open
          // bufferevent_free(bev);
          free(event_client->buffer);
          delete (event_client);
        }

      } else {
        break;
      }
    }
  }
}

void errorcb(bufferevent *bev, short events, void *ctx) {
  if (events & BEV_EVENT_EOF) {
    printf("Connection closed.\n");
  } else if (events & BEV_EVENT_ERROR) {
    printf("Got an error on the connection: %s\n",
           strerror(errno)); /*XXX win32*/
  }

  bufferevent_free(bev);
}

void do_accept(evutil_socket_t listener, short event, void *arg) {
  printf("server fd become readable\n");
  event_base *base = (event_base *)arg;

  sockaddr_storage ss;
  socklen_t slen = sizeof(ss);
  int fd = accept(listener, (sockaddr *)&ss, &slen);
  assert(fd >= 0);

  evutil_make_socket_nonblocking(fd);
  bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  assert(bev);

  auto event_client = new EventClient();

  bufferevent_setcb(bev, read_cb, NULL, errorcb, event_client);
  // bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
  bufferevent_enable(bev, EV_READ | EV_WRITE);
}

int main(int argc, char *argv[]) {

  Option option;

  if (parse_command_line(argc, argv, &option)) {
    if (option.receive) {
      int listener_fd = get_listen_fd(option.port);
      evutil_make_socket_nonblocking(listener_fd);

      event_base *base = event_base_new();
      assert(base);

      event *signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
      assert(signal_event);
      event_add(signal_event, NULL);

      event *listener_event = event_new(base, listener_fd, EV_READ | EV_PERSIST,
                                        do_accept, (void *)base);
      assert(listener_event);
      event_add(listener_event, NULL);

      printf("listening on port = %d, looping\n", option.port);
      event_base_dispatch(base);

      close(listener_fd);
    } else {
      assert(0);
    }
  }

  return 0;
}