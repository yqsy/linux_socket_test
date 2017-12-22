#include <arpa/inet.h>  // uint16_t
#include <assert.h>     // assert
#include <errno.h>      // perror
#include <netinet/in.h> // sockaddr_in
#include <signal.h>     // signal
#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <strings.h>    // bzero
#include <sys/socket.h> // socket
#include <sys/types.h> // some historical (BSD) implementations required this header file
#include <unistd.h> // close

#include <algorithm>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/noncopyable.hpp>

#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/net/Buffer.h>

#include <http/http_context.h>
#include <http/http_request.h>
#include <http/http_response.h>

// RAII
class Socket : boost::noncopyable
{
public:
  explicit Socket() : sockfd_(0) {}

  ~Socket() { close(sockfd_); }

public:
  int sockfd_;
};

typedef std::shared_ptr<Socket> SocketPtr;

string raw_str(const string &ori)
{
  string result;
  for (auto &c : ori)
  {
    if (c == '\r')
    {
      result += "\\r";
    }
    else if (c == '\n')
    {
      result += "\\n";
    }
    else
    {
      result += c;
    }
  }
  return result;
}

void shutdown_close_fd(int client_fd)
{
  if (shutdown(client_fd, SHUT_WR) < 0)
  {
    LOG_ERROR << "shutdown" << strerror(errno);
  }

  char ignore[1024];
  while (read(client_fd, ignore, sizeof(ignore)) > 0)
  {
    // do nothing
  }
}

int write_n(int sockfd, const void *buf, int length)
{
  int written = 0;
  while (written < length)
  {
    ssize_t nw = write(sockfd, static_cast<const char *>(buf) + written,
                       length - written);
    if (nw > 0)
    {
      written += static_cast<int>(nw);
    }
    else if (nw == 0)
    {
      break; // EOF
    }
    else if (errno != EINTR)
    {
      LOG_ERROR << "write" << strerror(errno);
      break;
    }
  }
  return written;
}

bool do_with_buffer(muduo::net::Buffer *buf, HttpContext *context,
                    SocketPtr client_socket)
{
  if (!context->parse_request(buf))
  {
    LOG_WARN << "parse error shutdown buf[:10] = "
             << raw_str(string(buf->peek(),
                               std::min(size_t(10), buf->readableBytes())));
    string err("HTTP/1.1 400 Bad Request\r\n\r\n");
    write_n(client_socket->sockfd_, err.c_str(), err.size());
    shutdown_close_fd(client_socket->sockfd_);
    return false;
  }

  if (context->got_all())
  {
    const auto &request = context->request();

    const string &connection = request.get_header("Connection");
    bool close =
        connection == "close" || (request.version() == HttpRequest::kHttp10 &&
                                  connection != "Keep-Alive");

    HttpResponse response(close);
    // response.set_status_code(HttpResponse::k404NotFound);
    // response.set_status_message("Not Found");
    // response.set_close_connection(true);

    response.set_status_code(HttpResponse::k200Ok);
    response.set_status_message("OK");
    response.set_content_type("text/plain");
    response.set_body("hello world");

    Buffer outbuf;
    response.append_to_buffer(&outbuf);

    int wn =
        write_n(client_socket->sockfd_, outbuf.peek(), outbuf.readableBytes());

    if (wn < 0)
    {
      LOG_ERROR << "write" << strerror(errno);
    }
    else if (static_cast<size_t>(wn) != outbuf.readableBytes())
    {
      LOG_ERROR << "wn = " << wn << " != " << outbuf.readableBytes();
    }

    outbuf.retrieveAll();

    if (response.close_connection())
    {
      shutdown_close_fd(client_socket->sockfd_);
    }

    context->reset();
  }

  return true;
}

void accept_request(SocketPtr client_socket)
{
  muduo::net::Buffer buf;
  HttpContext context;
  while (true)
  {
    int saved_errno = 0;
    ssize_t nr = buf.readFd(client_socket->sockfd_, &saved_errno);

    if (nr > 0)
    {
      if (!do_with_buffer(&buf, &context, client_socket))
      {
        break;
      }
    }
    else if (nr == 0)
    {
      // recv end of file, peer shutdown
      if (buf.readableBytes() > 0)
      {
        LOG_WARN << "remain " << buf.readableBytes() << "bytes";
      }
      break;
    }
    else // nr < 0
    {
      if (saved_errno != EINTR)
      {
        // system error
        LOG_ERROR << "read" << strerror(saved_errno);
        break;
      }
    }
  }
}

int get_listen_fd_or_die(uint16_t port)
{
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0)
  {
    LOG_FATAL << "socket" << strerror(errno);
  }

  // SO_REUSEADDR
  int yes = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
  {
    LOG_FATAL << "setsockopt" << strerror(errno);
  }

  // SIGPIPE
  signal(SIGPIPE, SIG_IGN);

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(listenfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)))
  {
    LOG_FATAL << "bind" << strerror(errno);
  }

  if (listen(listenfd, 5))
  {
    LOG_FATAL << "listen" << strerror(errno);
  }

  return listenfd;
}

int main(int argc, char *argv[])
{
  muduo::TimeZone beijing(8 * 3600, "CST");
  muduo::Logger::setTimeZone(beijing);

  if (argc < 2)
  {
    printf("Usage:\n%s port\n", argv[0]);
    exit(1);
  }

  uint16_t port = atoi(argv[1]);

  Socket listen_socket;
  listen_socket.sockfd_ = get_listen_fd_or_die(port);

  LOG_INFO << "listening on port = " << port;

  while (1)
  {
    struct sockaddr_in peer_addr;
    bzero(&peer_addr, sizeof(peer_addr));
    socklen_t addrlen = 0;

    SocketPtr client_socket = std::make_shared<Socket>();

    client_socket->sockfd_ =
        accept(listen_socket.sockfd_,
               reinterpret_cast<struct sockaddr *>(&peer_addr), &addrlen);
    if (client_socket->sockfd_ < 0)
    {
      LOG_FATAL << "accept" << strerror(errno);
    }

    std::thread thr(accept_request, client_socket);
    thr.detach();
  }

  return 0;
}