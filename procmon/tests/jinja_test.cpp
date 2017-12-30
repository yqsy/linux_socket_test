#include <errno.h>

#include <iostream>
#include <string>

#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>

#include <Jinja2CppLight/Jinja2CppLight.h>

using namespace std;
using namespace Jinja2CppLight;

int main(int argc, char *argv[])
{
  muduo::TimeZone beijing(8 * 3600, "CST");
  muduo::Logger::setTimeZone(beijing);

  // const string source = ("Hello {{name}}!"
  //                             ""
  //                             "{% if test %}"
  //                             "    How are you?"
  //                             "{% endif %}");
  // Template mytemplate(source);
  // mytemplate.setValue("name", "yqsy");
  // // mytemplate.setValue("test", 1);

  // const string result = mytemplate.render();
  // std::cout << "[" << result << "]" << endl;

  FILE *fp = fopen("procmondocs/procmon.html", "rb");
  if (fp)
  {
    const int kBufSize = 64 * 1024;
    char buf[kBufSize];
    size_t nread = fread(buf, 1, sizeof(buf), fp);

    if (nread <= 0)
    {
      LOG_FATAL << "read <=0 " << strerror(errno);
    }

    Template mytemplate(buf);
    mytemplate.setValue("procname", "proc1");
    mytemplate.setValue("hostname", "host1");
    std::cout << mytemplate.render();

    fclose(fp);
  }
  else
  {
    LOG_FATAL << "can not open html " << strerror(errno);
  }

  return 0;
}