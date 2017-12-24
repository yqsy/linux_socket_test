#include <stdio.h>
#include <stdlib.h>

#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <http/http_context.h>
#include <http/http_request.h>
#include <http/http_response.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

using namespace muduo;
using namespace muduo::net;

class Procmon : boost::noncopyable
{
public:
  void on_message(const TcpConnectionPtr &conn, Buffer *buf,
                  Timestamp receiveTime)
  {
    HttpContext *context =
        boost::any_cast<HttpContext>(conn->getMutableContext());

    if (!context->parse_request(buf))
    {
      conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
      conn->shutdown();
    }

    if (context->got_all())
    {
      const auto &request = context->request();

      const string &connection = request.get_header("Connection");
      bool close =
          connection == "close" || (request.version() == HttpRequest::kHttp10 &&
                                    connection != "Keep-Alive");

      HttpResponse response(close);
      on_request(request, &response);

      Buffer buf;
      response.append_to_buffer(&buf);
      conn->send(&buf);

      if (response.close_connection())
      {
        conn->shutdown();
      }

      context->reset();
    }
  }

  void on_connection(const TcpConnectionPtr &conn)
  {
    if (conn->connected())
    {
      conn->setContext(HttpContext());
    }
  }

  void on_request(const HttpRequest &req, HttpResponse *resp)
  {
    resp->set_status_code(HttpResponse::k200Ok);
    resp->set_status_message("OK");
    resp->set_content_type("text/plain");
    resp->add_header("Server", "yqsy-procmon");
  }
};

int main(int argc, char *argv[])
{
  muduo::TimeZone beijing(8 * 3600, "CST");
  muduo::Logger::setTimeZone(beijing);

  if (argc < 3)
  {
    printf("Usage: %s pid port\n", argv[0]);
    exit(1);
  }

  uint16_t port = atoi(argv[2]);

  LOG_INFO << "listening on port = " << port;

  Procmon procmon;
  EventLoop loop;
  TcpServer tcp_server(&loop, InetAddress(port), "procmon");

  tcp_server.setConnectionCallback(
      boost::bind(&Procmon::on_connection, &procmon, _1));

  tcp_server.setMessageCallback(
      boost::bind(&Procmon::on_message, &procmon, _1, _2, _3));

  tcp_server.start();
  loop.loop();

  return 0;
}
