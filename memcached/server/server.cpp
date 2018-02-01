#include <array>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_set>

#include <muduo/base/StringPiece.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/inspect/Inspector.h>

#include <boost/functional/hash/hash.hpp>
#include <boost/noncopyable.hpp>
#include <boost/program_options.hpp>

using namespace muduo;
using namespace muduo::net;

// po
uint16_t INSPECT_PORT = 11212;
uint16_t MEMCACHED_PORT = 11211;
int THREADS = 4;

// shared data
class Item : boost::noncopyable
{
public:
  Item(StringPiece keyArg)
      : hash_(boost::hash_range(keyArg.begin(), keyArg.end()))
  {
  }

  size_t hash() const { return hash_; }

  StringPiece key() const { return "key"; }

private:
  size_t hash_;
};

typedef std::shared_ptr<const Item> ConstItemPtr;

struct Hash
{
  size_t operator()(const ConstItemPtr &x) const { return x->hash(); }
};

struct Equal
{
  bool operator()(const ConstItemPtr &x, const ConstItemPtr &y) const
  {
    return x->key() == y->key();
  }
};

typedef std::unordered_set<ConstItemPtr, Hash, Equal> ItemMap;

struct MapWithLock
{
  std::mutex mtx;
  ItemMap items;
};

const int kShards = 4096;
std::array<MapWithLock, kShards> shareds;

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
