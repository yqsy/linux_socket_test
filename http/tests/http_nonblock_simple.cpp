#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <http/http_context.h>
#include <http/http_request.h>
#include <http/http_response.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

void on_connection(const TcpConnectionPtr &conn)
{
  if (conn->connected())
  {
    conn->setContext(HttpContext());
  }
}

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
    response.set_status_code(HttpResponse::k200Ok);
    response.set_status_message("OK");
    response.set_content_type("text/html");
    response.set_body("<P>hello world");

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

int main(int argc, char *argv[])
{
  muduo::TimeZone beijing(8 * 3600, "CST");
  muduo::Logger::setTimeZone(beijing);

  LOG_INFO << "listening on port = " << 80;

  EventLoop loop;
  TcpServer tcp_server(&loop, InetAddress(80), "helloworld");
  tcp_server.setConnectionCallback(boost::bind(&on_connection, _1));
  tcp_server.setMessageCallback(boost::bind(&on_message, _1, _2, _3));
  tcp_server.start();
  loop.loop();
  return 0;
}
