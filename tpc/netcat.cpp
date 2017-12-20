#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <thread>

#include <boost/asio.hpp>

using boost::asio::ip::address;
using boost::asio::ip::tcp;

int write_n(int fd, const void *buf, int length)
{
  int written = 0;
  while (written < length)
  {
    int nw =
        ::write(fd, static_cast<const char *>(buf) + written, length - written);
    if (nw > 0)
    {
      written += nw;
    }
    else if (nw == 0)
    {
      break; // EOF
    }
    else if (errno != EINTR)
    {
      perror("write");
      break;
    }
  }
  return written;
}

void run(tcp::socket *socket)
{

  std::thread thr([&socket] {
    char buf[8192];
    int nr = 0;

    boost::system::error_code ignore_err;
    while ((nr = socket->read_some(boost::asio::buffer(buf, sizeof(buf)),
                                   ignore_err)) > 0)
    {

      assert(nr > 0);

      int nw = write_n(STDOUT_FILENO, buf, nr);
      assert(nw > 0);
      //   if (nw < nr) {
      //     break;
      //   }
    }

    ::exit(0);
  });

  char buf[8192];
  int nr = 0;
  while ((nr = ::read(STDIN_FILENO, buf, sizeof(buf))) > 0)
  {
    boost::system::error_code ec;
    size_t nw = boost::asio::write(*socket, boost::asio::buffer(buf, nr), ec);
    if (ec)
    {
      std::cout << ec.message() << std::endl;
      break;
    }
    assert(nw == static_cast<size_t>(nr)); // must > 0
  }
  socket->shutdown(tcp::socket::shutdown_send);
  thr.join();
}

void server(uint16_t port)
{
  printf("listening on port = %d\n", port);
  boost::asio::io_service io_service;
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
  tcp::socket socket(io_service);
  acceptor.accept(socket);
  run(&socket);
}

void client(const char *hostname, uint16_t port)
{
  boost::asio::io_service io_service;
  tcp::socket socket(io_service);

  tcp::resolver resolver(io_service);
  tcp::resolver::query query(tcp::v4(), hostname, "");

  auto iter = resolver.resolve(query);

  assert(iter != tcp::resolver::iterator());

  tcp::endpoint endpoint = *iter;
  endpoint.port(port);

  std::cout << "connecting to " << endpoint << std::endl;

  boost::system::error_code ec;
  socket.connect(endpoint, ec);
  if (ec)
  {
    std::cout << ec.message() << std::endl;
    exit(1);
  }
  run(&socket);
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Usage:\n %s hostname port\n %s -l port\n", argv[0], argv[0]);
    return 0;
  }

  uint16_t port = atoi(argv[2]);

  if (strcmp(argv[1], "-l") == 0)
  {
    server(port);
  }
  else
  {
    client(argv[1], port);
  }

  return 0;
}