#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ctime>
#include <iostream>
#include <string>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

std::string make_daytime_string() {
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

int16_t PORT = 5002;

void sender(const char *filename, tcp::socket *socket) {

  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    return;
  }

  printf("Sleep 10 seconds.\n");
  sleep(10);

  printf("Start sending file %s\n", filename);
  char buf[8192];
  size_t nr = 0;
  while ((nr = fread(buf, 1, sizeof(buf), fp)) > 0) {
    boost::system::error_code ec;
    boost::asio::write(*socket, boost::asio::buffer(buf, nr), ec);
    if (ec) {
      std::cout << ec.message() << std::endl;
      exit(1);
    }
  }

  fclose(fp);
  printf("Finish sending file %s\n", filename);

  // Safe close connection
  printf("Shutdown write and read until EOF\n");
  socket->shutdown(tcp::socket::shutdown_send);

  boost::system::error_code ignored_error;
  while (socket->read_some(boost::asio::buffer(buf), ignored_error) > 0) {
  }

  printf("All done.\n");
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    printf("Usage: \n %s filename\n", argv[0]);
    return 0;
  }

  printf("listening on port = %d\n", PORT);

  boost::asio::io_service io_service;
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), PORT));

  for (;;) {
    tcp::socket socket(io_service);
    acceptor.accept(socket);

    sender(argv[1], &socket);
  }

  return 0;
}