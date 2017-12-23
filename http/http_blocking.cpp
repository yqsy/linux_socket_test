#include <arpa/inet.h>  // uint16_t
#include <assert.h>     // assert
#include <errno.h>      // perror
#include <netinet/in.h> // sockaddr_in
#include <signal.h>     // signal
#include <stdio.h>      // printf
#include <stdlib.h>     // exit
#include <strings.h>    // bzero
#include <sys/socket.h> // socket
#include <sys/stat.h>
#include <sys/types.h> // some historical (BSD) implementations required this header file
#include <sys/types.h>
#include <sys/wait.h>
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

void unimplemented(HttpResponse *response)
{
  response->set_status_code(HttpResponse::k501NotImplemented);
  response->set_status_message("Method Not Implemented");
  response->set_content_type("text/html");
  response->set_body("<HTML><HEAD><TITLE>Method Not Implemented"
                     "</TITLE></HEAD>"
                     "<BODY><P>HTTP request method not supported."
                     "</BODY></HTML>");
}

void not_found(HttpResponse *response)
{
  response->set_status_code(HttpResponse::k404NotFound);
  response->set_status_message("NOT FOUND");
  response->set_content_type("text/html");
  response->set_body("<HTML><TITLE>Not Found</TITLE>"
                     "<BODY><P>The server could not fulfill"
                     "your request because the resource specified"
                     "is unavailable or nonexistent."
                     "</BODY></HTML>");
}

void cannot_execute(HttpResponse *response)
{
  response->set_status_code(HttpResponse::k500Error);
  response->set_status_message("NOT FOUND");
  response->set_content_type("text/html");
  response->set_body("<P>Error prohibited CGI execution.");
}

void serve_file(const string &file, FILE *fp, HttpResponse *response)
{
  response->set_status_code(HttpResponse::k200Ok);
  response->set_status_message("OK");
  response->set_content_type("text/html");

  char buf[8192];

  size_t nr = 0;
  while ((nr = fread(buf, 1, sizeof(buf), fp)) > 0)
  {
    // FIXME: 暂时就拷到缓冲区把
    response->append_body(string(buf, nr));
  }
}

void server_cgi(const HttpRequest &request, HttpResponse *response)
{
  response->set_status_code(HttpResponse::k200Ok);
  response->set_status_message("OK");
  // FIXME: CGI set  content_type
  response->set_content_type("text/html");

  int cgi_output[2]; // cgi -> parent
  int cgi_input[2];  // parent -> cgi

  if (pipe(cgi_output) < 0 || pipe(cgi_input) < 0)
  {
    cannot_execute(response);
    return;
  }
  int pid;

  if ((pid = fork()) < 0)
  {
    cannot_execute(response);
    return;
  }

  if (pid == 0)
  {
    // child
    dup2(cgi_output[1], STDOUT_FILENO);
    dup2(cgi_input[0], STDIN_FILENO);
    close(cgi_output[0]);
    close(cgi_input[1]);

    char meth_env[255];
    snprintf(meth_env, sizeof(meth_env), "REQUEST_METHOD=%s",
             request.method_str().c_str());
    putenv(meth_env);

    if (request.method() == HttpRequest::kGet)
    {
      // TODO
      // char query_env[255];
      // snprinf(query_env, sizeof(query_env), "QUERY_STRING=%s", );
    }
    else if (request.method() == HttpRequest::kPost)
    {
      char length_env[255];
      snprintf(length_env, sizeof(length_env), "CONTENT_LENGTH=%zu",
               request.body().size());
      putenv(length_env);
    }

    string request_path = string("web") + request.path();
    execl(request_path.c_str(), request_path.c_str(), NULL);
    exit(0);
  }
  else
  {
    // parent
    close(cgi_output[1]);
    close(cgi_input[0]);
    if (request.method() == HttpRequest::kPost)
    {
      write(cgi_input[1], request.body().c_str(), request.body().size());
    }

    char buf[8192];
    int nr;
    while ((nr = read(cgi_output[0], buf, sizeof(buf))) > 0)
    {
      response->append_body(string(buf, nr));
    }
    close(cgi_output[0]);
    close(cgi_input[1]);
    int status;
    waitpid(pid, &status, 0);
  }
}

HttpResponse deal_with_request(const HttpRequest &request)
{
  const string &connection = request.get_header("Connection");
  bool close =
      connection == "close" ||
      (request.version() == HttpRequest::kHttp10 && connection != "Keep-Alive");

  HttpResponse response(close);

  if (!(request.method() == HttpRequest::kGet ||
        request.method() == HttpRequest::kPost))
  {
    unimplemented(&response);
    return response;
  }

  if (request.method() == HttpRequest::kPost)
  {
    // cgi
    server_cgi(request, &response);
    return response;
  }

  if (request.method() == HttpRequest::kGet)
  {
    if (request.query() != "")
    {
      // cgi
      unimplemented(&response);
      return response;
    }
    else
    {
      // get static file
      // FIXME: 全局同一的web目录
      string request_path = string("web") + request.path();
      if (request_path[request_path.size() - 1] == '/')
      {
        request_path += "index.html";
      }

      struct stat st;
      if (stat(request_path.c_str(), &st) == -1)
      {
        LOG_WARN << "file:" << request_path << " stat " << strerror(errno);
        not_found(&response);
        return response;
      }

      if ((st.st_mode & S_IFMT) == S_IFDIR)
      {
        request_path += "/index.html";
      }

      FILE *fp = fopen(request_path.c_str(), "rb");

      if (!fp)
      {
        LOG_WARN << "file: " << request_path << " can not fopen";
        not_found(&response);
        return response;
      }

      serve_file(request_path, fp, &response);
      fclose(fp);
      return response;
    }
  }
  return response;
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

    auto response = deal_with_request(request);
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