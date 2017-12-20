
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <event2/event.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 16384

void do_read(evutil_socket_t fd, short events, void *arg);
void do_write(evutil_socket_t fd, short events, void *arg);

struct fd_state
{
  char buffer[MAX_LINE];
  size_t buffer_used;

  size_t n_written;
  size_t write_upto;

  event *read_event;
  event *write_event;
};

char rot13_char(char c)
{
  if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
  {
    return c + 13;
  }
  else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
  {
    return c - 13;
  }
  else
  {
    return c;
  }
}

fd_state *alloc_fd_state(event_base *base, evutil_socket_t fd)
{
  fd_state *state = (fd_state *)malloc(sizeof(fd_state));
  if (!state)
  {
    return NULL;
  }

  state->read_event = event_new(base, fd, EV_READ | EV_PERSIST, do_read, state);
  if (!state->read_event)
  {
    free(state);
    return NULL;
  }

  state->write_event =
      event_new(base, fd, EV_WRITE | EV_PERSIST, do_write, state);
  if (!state->write_event)
  {
    event_free(state->read_event);
    free(state);
    return NULL;
  }
  state->buffer_used = state->n_written = state->write_upto = 0;
  return state;
}

void free_fd_state(fd_state *state)
{
  event_free(state->read_event);
  event_free(state->write_event);
  free(state);
}

void do_read(evutil_socket_t fd, short events, void *arg)
{
  fd_state *state = (fd_state *)arg;
  char buf[1024] = {};

  ssize_t result = 0;
  while (1)
  {
    result = recv(fd, buf, sizeof(buf), 0);
    if (result <= 0)
    {
      break;
    }

    for (int i = 0; i < result; ++i)
    {
      if (state->buffer_used < sizeof(state->buffer))
      {
        // buffer_used 当前已经使用了的字节数等于下一个元素的下标值
        state->buffer[state->buffer_used++] = rot13_char(buf[i]);
      }

      if (buf[i] == '\n')
      {
        event_add(state->write_event, NULL);
        // 需要一次性发送的字节数
        state->write_upto = state->buffer_used;
      }
    }
  }

  if (result == 0)
  {
    free_fd_state(state);
  }
  else if (result < 0)
  {
    if (errno == EAGAIN)
    {
      return;
    }
    perror("recv");
    free_fd_state(state);
  }
}

void do_write(evutil_socket_t fd, short events, void *arg)
{
  fd_state *state = (fd_state *)arg;

  // 循环到 写入字节数 == 一次性发送的字节数 为止
  while (state->n_written < state->write_upto)
  {

    ssize_t result =
        send(fd, state->buffer + state->n_written /*下一发送字节的index*/,
             state->write_upto - state->n_written, 0);

    if (result < 0)
    {
      if (errno == EAGAIN)
      {
        return;
      }
      free_fd_state(state);
      return;
    }
    assert(result != 0);

    state->n_written += result;
  }

  assert(state->n_written == state->buffer_used);
  if (state->n_written == state->buffer_used)
  {
    // state->n_written = state->write_upto = state->buffer_used = 1;
    // 这里为什么是1?
    state->n_written = state->write_upto = state->buffer_used = 0;
  }
  event_del(state->write_event);
}

void do_accept(evutil_socket_t listener, short event, void *arg)
{
  event_base *base = (event_base *)arg;
  sockaddr_storage ss;

  socklen_t slen = sizeof(ss);
  int fd = accept(listener, (sockaddr *)&ss, &slen);
  if (fd < 0)
  {
    perror("accept");
  }
  else if (fd > FD_SETSIZE /*1024?*/)
  {
    close(fd);
  }
  else
  {
    evutil_make_socket_nonblocking(fd);
    fd_state *state = alloc_fd_state(base, fd);
    assert(state);
    assert(state->read_event);
    assert(state->write_event);
    event_add(state->read_event, NULL);
  }
}

void run()
{
  sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(40713);

  evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener);

  int one = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  if (bind(listener, (sockaddr *)&sin, sizeof(sin)) < 0)
  {
    perror("bind");
    return;
  }

  if (listen(listener, 16) < 0)
  {
    perror("listen");
    return;
  }

  event_base *base = event_base_new();
  if (!base)
  {
    return;
  }

  event *listener_event =
      event_new(base, listener, EV_READ | EV_PERSIST, do_accept, (void *)base);
  event_add(listener_event, NULL);

  event_base_dispatch(base);
}

int main(int argc, char *argv[])
{
  setvbuf(stdout, NULL, _IONBF, 0);

  run();
  return 0;
}