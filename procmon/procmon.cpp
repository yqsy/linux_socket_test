#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <streambuf>
#include <vector>

#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/format.hpp>
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

#include <procmon/plot.h>

using namespace muduo;
using namespace muduo::net;
using namespace Jinja2CppLight;

struct StatData
{
  void parse(const char *start_at_state, int kb_per_page)
  {
    std::istringstream iss(start_at_state);

    iss >> state;
    iss >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags;
    iss >> minflt >> cminflt >> majflt >> cmajflt;
    iss >> utime >> stime >> cutime >> cstime;
    iss >> priority >> nice >> num_threads >> itrealvalue >> starttime;
    long vsize, rss;
    iss >> vsize >> rss >> rsslim;
    vsizeKb = vsize / 1024;
    rssKb = rss * kb_per_page;
  }

  char state;
  int ppid;
  int pgrp;
  int session;
  int tty_nr;
  int tpgid;
  int flags;

  long minflt;
  long cminflt;
  long majflt;
  long cmajflt;

  long utime;
  long stime;
  long cutime;
  long cstime;

  long priority;
  long nice;
  long num_threads;
  long itrealvalue;
  long starttime;

  long vsizeKb;
  long rssKb;
  long rsslim;
};

struct CpuTime
{
  int user_time_;
  int sys_time_;

  double cpu_usage(double kperiod, double kclock_ticks_per_seconds) const
  {
    return (user_time_ + sys_time_) / (kperiod * kclock_ticks_per_seconds);
  }
};

class Procmon : boost::noncopyable
{
public:
  Procmon(pid_t pid)
      : kclock_ticks_per_second_(muduo::ProcessInfo::clockTicksPerSecond()),
        kb_per_page_(muduo::ProcessInfo::pageSize() / 1024),
        kboot_time_(get_boot_time()), // epoch time
        beijing_(8 * 3600, "CST"), pid_(pid),
        procname_(ProcessInfo::procname(read_proc_file("stat")).as_string()),
        hostname_(ProcessInfo::hostname()), cmd_line_(get_cmd_line()),
        ticks_(0), cpu_usage_(600 / kperiod_),
        cpu_chart_(640, 100, 600, kperiod_) // 10 minutes
  {
    bzero(&last_stat_data_, sizeof(last_stat_data_));
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
    else if (req.path() == "/cmdline")
    {
      resp->set_body(cmd_line_);
    }
    else if (req.path() == "/environ")
    {
      resp->set_body(get_environ());
    }
    else if (req.path() == "/threads")
    {
      resp->set_body("");
    }
    else if (req.path() == "/io")
    {
      resp->set_body(read_proc_file("io"));
    }
    else if (req.path() == "/limits")
    {
      resp->set_body(read_proc_file("limits"));
    }
    else if (req.path() == "/maps")
    {
      resp->set_body(read_proc_file("maps"));
    }
    else if (req.path() == "/smaps")
    {
      resp->set_body(read_proc_file("smaps"));
    }
    else if (req.path() == "/status")
    {
      resp->set_body(read_proc_file("status"));
    }
    else if (req.path() == "/cpu.png")
    {
      std::vector<double> cpu_usage;
      for (size_t i = 0; i < cpu_usage_.size(); ++i)
      {
        cpu_usage.push_back(
            cpu_usage_[i].cpu_usage(kperiod_, kclock_ticks_per_second_));
      }

      string png = cpu_chart_.plotCpu(cpu_usage);
      resp->set_body(png);
      resp->set_content_type("image/png");
    }
    else
    {
      resp->set_status_code(HttpResponse::k404NotFound);
      resp->set_status_message("Not Found");
      resp->set_close_connection(true);
    }
  }

  void fill_over_fiew(const string &query, HttpResponse *resp)
  {
    string stat = read_proc_file("stat");

    if (stat.empty())
    {
      std::ifstream ifs("procmondocs/nostat.html");

      string str((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());

      Template t(str);
      t.setValue("procname", procname_);
      t.setValue("hostname", hostname_);
      t.setValue("pid", std::to_string(pid_));

      resp->set_body(t.render());
      return;
    }

    Timestamp now = Timestamp::now();

    pid_t pid = atoi(stat.c_str());
    assert(pid == pid_);

    std::ifstream ifs("procmondocs/procmon.html");
    string str((std::istreambuf_iterator<char>(ifs)),
               std::istreambuf_iterator<char>());
    Template t(str);

    // refresh
    size_t pos = query.find("refresh=");
    if (pos != string::npos)
    {
      int seconds = atoi(query.c_str() + pos + 8);
      if (seconds > 0)
      {
        t.setValue("refresh", std::to_string(seconds));
      }
    }

    t.setValue("procname", procname_);
    t.setValue("hostname", hostname_);
    t.setValue("nowtime", format_cst(now));
    t.setValue("pid", std::to_string(pid_));

    StatData stat_data;
    bzero(&stat_data, sizeof(stat_data));
    StringPiece procname = ProcessInfo::procname(stat);
    assert(*procname.end() == ')');

    stat_data.parse(procname.end() + 1, kb_per_page_);
    Timestamp started(get_start_time(stat_data.starttime));
    t.setValue("start_time", format_cst(started, false));
    t.setValue("uptimes", format_double(timeDifference(now, started)));
    t.setValue("ececuable", read_link("exe"));
    t.setValue("current_dir", read_link("cwd"));
    t.setValue("state", get_state(stat_data.state));
    t.setValue("user_time", format_double(get_seconds(stat_data.utime)));
    t.setValue("system_time", format_double(get_seconds(stat_data.stime)));
    t.setValue("vm_size", std::to_string(stat_data.vsizeKb));
    t.setValue("vm_rss", std::to_string(stat_data.rssKb));
    t.setValue("threads", std::to_string(stat_data.num_threads));
    t.setValue("priority", std::to_string(stat_data.priority));
    t.setValue("nice", std::to_string(stat_data.nice));
    t.setValue("minor_page_faults", std::to_string(stat_data.minflt));
    t.setValue("major_page_faults", std::to_string(stat_data.majflt));
    resp->set_body(t.render());
  }

  string format_cst(const Timestamp &stamp, bool showMicroseconds = true) const
  {
    struct tm tm_time = beijing_.toLocalTime(stamp.secondsSinceEpoch());

    char buf[32] = {0};
    if (showMicroseconds)
    {
      int microseconds = static_cast<int>(stamp.microSecondsSinceEpoch() %
                                          Timestamp::kMicroSecondsPerSecond);
      snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
               tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
               tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
    }
    else
    {
      snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
               tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
               tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
  }

  static string format_double(double val)
  {
    return boost::str(boost::format("%.2f") % val);
  }

  static const char *get_state(char state)
  {
    switch (state)
    {
    case 'R':
      return "Running";
    case 'S':
      return "Sleeping";
    case 'D':
      return "Disk sleep";
    case 'Z':
      return "Zombie";
    default:
      return "Unknown";
    }
  }

  string read_proc_file(const char *basename) const
  {
    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/%s", pid_, basename);
    string content;
    FileUtil::readFile(filename, 1024 * 1024, &content);
    return content;
  }

  string read_link(const char *basename) const
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

  string get_cmd_line() const
  {
    return boost::replace_all_copy(read_proc_file("cmdline"), string(1, '\0'),
                                   "\n\t");
  }

  string get_environ() const
  {
    return boost::replace_all_copy(read_proc_file("environ"), string(1, '\0'),
                                   "\n");
  }

  // starttime = /porc/pid/stat starttime
  // 应该是进程启动到现在的tick数量
  // 需要转换成second
  // 返回启动时间的epoch time
  Timestamp get_start_time(long starttime) const
  {
    return Timestamp(Timestamp::kMicroSecondsPerSecond * kboot_time_ +
                     Timestamp::kMicroSecondsPerSecond * starttime /
                         kclock_ticks_per_second_);
  }

  double get_seconds(long ticks) const
  {
    return static_cast<double>(ticks) / kclock_ticks_per_second_;
  }

  long get_long(const string &status, const char *key) const
  {
    long result = 0;
    size_t pos = status.find(key);
    if (pos != string::npos)
    {
      result = atol(status.c_str() + pos + strlen(key));
    }
    return result;
  }

  long get_boot_time() const
  {
    string stat;
    FileUtil::readFile("/proc/stat", 65536, &stat);
    return get_long(stat, "btime ");
  }

  void tick()
  {
    string stat = read_proc_file("stat");
    if (stat.empty())
    {
      return;
    }

    StringPiece procname = ProcessInfo::procname(stat);
    StatData stat_data;
    bzero(&stat_data, sizeof(stat_data));
    assert(*procname.end() == ')');
    stat_data.parse(procname.end() + 1, kb_per_page_);

    if (ticks_ > 0)
    {
      CpuTime time;
      time.user_time_ = std::max(
          0, static_cast<int>(stat_data.utime - last_stat_data_.utime));
      time.sys_time_ = std::max(
          0, static_cast<int>(stat_data.stime - last_stat_data_.stime));
      cpu_usage_.push_back(time);
    }

    last_stat_data_ = stat_data;
    ++ticks_;
  }

  int period() const { return kperiod_; }

private:
  const int kperiod_ = 2.0;           // cpu flush every two seconds
  const int kclock_ticks_per_second_; // get procerss start time
  const int kb_per_page_;             // parse /proc/pid/stat
  const long kboot_time_;             // epoch time
  const muduo::TimeZone beijing_;     // for time format

  const pid_t pid_;
  const string procname_;
  const string hostname_;
  const string cmd_line_;

  StatData last_stat_data_;
  int ticks_; // 记录两次间隔之内的cpu使用率
  boost::circular_buffer<CpuTime> cpu_usage_; // 10分钟内cpu统计
  Plot cpu_chart_;
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

  tcp_server.getLoop()->runEvery(procmon.period(),
                                 boost::bind(&Procmon::tick, &procmon));
  tcp_server.start();
  loop.loop();

  return 0;
}
