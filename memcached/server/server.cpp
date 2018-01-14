#include <stdio.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/inspect/Inspector.h>

#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>

uint16_t INSPECT_PORT = 11212;

int main(int argc, char *argv[])
{
  muduo::TimeZone beijing(8 * 3600, "CST");
  muduo::Logger::setTimeZone(beijing);

  muduo::net::EventLoopThread inspectThread;

  new muduo::net::Inspector(inspectThread.startLoop(),
                            muduo::net::InetAddress(INSPECT_PORT),
                            "memcached-muduo");

  getchar();

  return 0;
}