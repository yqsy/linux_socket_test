#include <memcached/server/memcached_server.h>

#include <iostream>
#include <cassert>

#include <muduo/base/Logging.h>




MemcachedServer::Options::Options()
{
    bzero(this, sizeof(*this));
}

MemcachedServer::MemcachedServer(muduo::net::EventLoop* loop, const Options& options)
    : loop_(loop),
      options_(options),
      server_(loop, muduo::net::InetAddress(options_.tcpPort), "memcached")
{
    using std::placeholders::_1;
    server_.setConnectionCallback(std::bind(&MemcachedServer::onConnection, this, _1));
}

void MemcachedServer::start()
{
    server_.start();
}

void MemcachedServer::setThreadNum(int threads)
{
    server_.setThreadNum(threads);
}

void MemcachedServer::storeItem(ConstItemPtr itemPtr)
{
    muduo::MutexLock& mutex = shards_[itemPtr->hash() % kShards].mutex;
    ItemMap& items = shards_[itemPtr->hash() % kShards].items;

    muduo::MutexLockGuard lockGuard(mutex);

    // if exist before, it will be replaced
    items.insert(itemPtr);
}


ConstItemPtr MemcachedServer::getItem(ConstItemPtr key)
{
    muduo::MutexLock& mutex = shards_[key->hash() % kShards].mutex;
    ItemMap& items = shards_[key->hash() % kShards].items;

    muduo::MutexLockGuard lockGuard(mutex);
    
    auto it = items.find(key);
    if (it != items.end())
    {
        return *it;
    }
    else
    {
        return ConstItemPtr();
    }
}

void MemcachedServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        SessionPtr session(new Session(this, conn));
        muduo::MutexLockGuard lock(mutex_);
        assert(sessions_.find(conn->name()) == sessions_.end());
        sessions_[conn->name()] = session;
    }
    else
    {
        muduo::MutexLockGuard lock(mutex_);
        assert(sessions_.find(conn->name()) != sessions_.end());
        sessions_.erase(conn->name());
    }
}
