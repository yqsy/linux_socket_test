#include <stdio.h>
#include <iostream>

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/inspect/Inspector.h>

#include <boost/program_options.hpp>

#include <memcached/server/memcached_server.h>

using namespace muduo;
using namespace muduo::net;
namespace po = boost::program_options;

int main(int argc, char* argv[]) 
{
    MemcachedServer::Options options;
    options.tcpPort = 11211;
    options.inspectPort = 11212;
    options.threads = 4;

    po::options_description desc("Allowed options");

    desc.add_options()
        ("help,h", "Help")
        ("port,p", po::value<uint16_t>(&options.tcpPort), "TCP port")
        ("threads,t", po::value<int>(&options.threads), "Threads")
        ("inspectPort", po::value<uint16_t>(&options.inspectPort), "Inspect port")
        ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 0;
    }

    EventLoop loop;
    EventLoopThread inspectThread;
    
    new Inspector(inspectThread.startLoop(), InetAddress(options.inspectPort), "inspect");

    MemcachedServer server(&loop, options);
    server.setThreadNum(options.threads);
    server.start();

    loop.loop();
    return 0;
}
