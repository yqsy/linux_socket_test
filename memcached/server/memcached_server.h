#pragma once

#include <strings.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>

#include <boost/noncopyable.hpp>

#include <muduo/net/TcpServer.h>
#include <muduo/base/Mutex.h>

#include <memcached/server/session.h>
#include <memcached/server/item.h>

struct Hash
{
    size_t operator() (const ConstItemPtr& x) const
    {
        return x->hash();
    }
};

struct Equal
{
    bool operator() (const ConstItemPtr& x, const ConstItemPtr& y) const
    {
        return x->key() == y->key();
    }
};

typedef std::unordered_set<ConstItemPtr, Hash, Equal> ItemMap;


struct MapWithLock
{
    ItemMap items;
    mutable muduo::MutexLock mutex;
};

class MemcachedServer : public boost::noncopyable
{
public:

    struct Options
    {
        Options();

        uint16_t tcpPort;
        uint16_t inspectPort;
        int threads;
    };

    MemcachedServer(muduo::net::EventLoop* loop, const Options& options);

    void start();

    void setThreadNum(int threads);

public:
    // for session to use

    // save for a certian item
    void storeItem(ConstItemPtr itemPtr);
    
    // create a struct with only key to query the hash table
    ConstItemPtr getItem(ConstItemPtr key);


private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn);


private:
    // session

    mutable muduo::MutexLock mutex_;
    std::unordered_map<std::string, SessionPtr> sessions_;

private:
    // hash bucket + hash table with lock

    const static int kShards = 4096;
    std::array<MapWithLock, kShards> shards_;

private:

    muduo::net::EventLoop* loop_;
    Options options_;
    muduo::net::TcpServer server_;
};

