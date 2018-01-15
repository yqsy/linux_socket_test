#include <array>

#include <muduo/net/inspect/Inspector.h>

#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>

#include <boost/program_options.hpp>

using namespace muduo;
using namespace muduo::net;

// po
uint16_t INSPECT_PORT = 11212;
uint16_t MEMCACHED_PORT = 11211;
int THREADS = 4;

// shared data
class Item
{
};

struct MapWithLock
{
  mutable muduo::MutexLock mutex;
};

int main(int argc, char *argv[])
{
  EventLoopThread inspectThread;

  new Inspector(inspectThread.startLoop(), InetAddress(INSPECT_PORT),
                "inspect");

  EventLoop loop;
  TcpServer server(&loop, InetAddress(MEMCACHED_PORT), "memcached-muduo");
  server.setThreadNum(THREADS);
  server.start();

  loop.loop();

  return 0;
}
