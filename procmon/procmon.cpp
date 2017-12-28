#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <streambuf>

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

#include <muduo/base/FileUtil.h>
#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>
#include <muduo/base/TimeZone.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <Jinja2CppLight/Jinja2CppLight.h>

#include <http/http_context.h>
#include <http/http_request.h>
#include <http/http_response.h>

using namespace muduo;
using namespace muduo::net;
using namespace Jinja2CppLight;

class Procmon : boost::noncopyable
{
public:
  Procmon(pid_t pid)
      : pid_(pid),
        procname_(ProcessInfo::procname(read_proc_file("stat")).as_string()),
        hostname_(ProcessInfo::hostname())
  {
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

    if (req.path() == "/")
    {
      resp->set_content_type("text/html");
      fill_over_fiew(req.query(), resp);
    }
  }

  void fill_over_fiew(const string &query, HttpResponse *resp)
  {
    string stat = read_proc_file("stat");

    if (stat.empty())
    {
      std::ifstream t("procmondocs/nostat.html");

      string str((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());

      Template t(str.c_str());
      t.setValue("procname", procname_.c_str());
      t.setValue("hostname", hostname_.c_str());
      t.setValue("pid", boost::lexical_cast<string>(pid_).c_str());
      resp->set_body(t.render().c_str());
      return;
    }

    Timestamp now = Timestamp::now()

        pid_t pid = atoi(stat.c_str());
    assert(pid == pid_);

    std::ifstream t("procmondocs/procmon.html");
    string str((std::istreambuf_iterator<char>(t)),
               std::istreambuf_iterator<char>());
    Template t(str.c_str());
    t.setValue("procname", procname_.c_str());
    t.setValue("hostname", hostname_.c_str());
    t.setValue("nowtime", now.toFormattedString().c_str());
    t.SetValue("pid", boost::lexical_cast<string>(pid_).c_str());

    Timestamp started(getStartTime(statData.starttime));

    t.SetValue("start_time", );
  }

  string read_proc_file(const char *basename)
  {
    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/%s", pid_, basename);
    string content;
    FileUtil::readFile(filename, 1024 * 1024, &content);
    return content;
  }

  string read_link(const char *basename)
  {
    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/%s", pid_, basename);
    char link[1024];
    ssize_t len = readlink(filename, link, sizeof(link));

    string result;
    if (len > 0)
    {
      result.assign(link, len);
    }
    return result;
  }

private:
  const pid_t pid_;

  const string procname_;
  const string hostname_;
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

  pid_t pid = atoi(argv[1]);
  uint16_t port = atoi(argv[2]);

  LOG_INFO << "listening on port = " << port;

  Procmon procmon(pid);
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
