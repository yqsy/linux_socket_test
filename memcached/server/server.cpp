#include <stdio.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/inspect/Inspector.h>

#include <muduo/base/Logging.h>
#include <muduo/base/TimeZone.h>

using namespace muduo::net;

uint16_t INSPECT_PORT = 11212;

int main(int argc, char *argv[])
{
  muduo::TimeZone beijing(8 * 3600, "CST");
  muduo::Logger::setTimeZone(beijing);

  EventLoopThread inspectThread;

  new Inspector(inspectThread.startLoop(), InetAddress(INSPECT_PORT),
                "memcached-muduo");

  getchar();

  return 0;
}