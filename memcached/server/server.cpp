#include <muduo/net/inspect/Inspector.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>

using namespace muduo;
using namespace muduo::net;

uint16_t INSPECT_PORT = 11212;

int main(int argc, char *argv[])
{
  EventLoop loop;
  EventLoopThread inspectThread;

  new Inspector(inspectThread.startLoop(), InetAddress(INSPECT_PORT),
                "memcached-muduo");

  loop.loop();

  return 0;
}
