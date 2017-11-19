#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINE 16384

char rot13_char(char c) {
  if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M')) {
    return c + 13;
  } else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z')) {
    return c - 13;
  } else {
    return c;
  }
}

void readcb(bufferevent *bev, void *ctx) {
  evbuffer *input = bufferevent_get_input(bev);
  evbuffer *output = bufferevent_get_output(bev);

  char *line;
  size_t n;
  while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF))) {
    for (int i = 0; i < n; ++i) {
      line[i] = rot13_char(line[i]);
    }
    evbuffer_add(output, line, n);
    evbuffer_add(output, "\n", 1);
    free(line);
  }

  if (evbuffer_get_length(input) >= MAX_LINE) {
    // Too long, just process what there is and go on so that the buffer
    // doesn't grow infinitely long
    char buf[1024] = {};
    while (evbuffer_get_length(input)) {
      int n = evbuffer_remove(input, buf, sizeof(buf));
      for (int i = 0; i < n; ++i) {
        buf[i] = rot13_char(buf[i]);
      }
      evbuffer_add(output, buf, n);
    }
    evbuffer_add(output, "\n", 1);
  }
}

void errorcb(bufferevent *bev, short error, void *ctx) {
  if (errno & BEV_EVENT_EOF) {
    // connection has been closed, do any clean up here
  } else if (error & BEV_EVENT_ERROR) {
    // check errno to see what error occurred
  } else if (error & BEV_EVENT_TIMEOUT) {
    // must be a timeout event handle, handle it
  }
  bufferevent_free(bev);
}

void do_accept(evutil_socket_t listener, short event, void *arg) {
  event_base *base = (event_base *)arg;
  sockaddr_storage ss;
  socklen_t slen = sizeof(ss);
  int fd = accept(listener, (sockaddr *)&ss, &slen);

  if (fd < 0) {
    perror("accept");
  } else if (fd > FD_SETSIZE) {
    close(fd);
  } else {
    evutil_make_socket_nonblocking(fd);
    bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
    bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
  }
}

void run() {
  sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(40713);

  evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener);

  int one = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  if (bind(listener, (sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("bind");
    return;
  }

  if (listen(listener, 16) < 0) {
    perror("listen");
    return;
  }

  event_base *base = event_base_new();
  if (!base) {
    return;
  }

  event *listener_event =
      event_new(base, listener, EV_READ | EV_PERSIST, do_accept, (void *)base);
  event_add(listener_event, NULL);

  event_base_dispatch(base);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, 0);

  run();
  return 0;
}