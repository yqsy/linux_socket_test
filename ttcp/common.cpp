#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/program_options.hpp>

#include "common.h"

namespace po = boost::program_options;

bool parse_command_line(int argc, char *argv[], Option *option) {

  po::options_description desc("Allowed options");

  desc.add_options()("help,h", "Help")(
      "port,p", po::value<uint16_t>(&option->port)->default_value(5001),
      "TCP port")("length,l",
                  po::value<int>(&option->length)->default_value(65536),
                  "Buffer length")(
      "number,n", po::value<int>(&option->number)->default_value(8192),
      "Number of buffers")("trans,t", "Transmit")("recv,r", "Receive")(
      "host", po::value<std::string>(&option->host)->default_value("127.0.0.1"),
      "Transmit to host");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  option->transmit = vm.count("trans");
  option->receive = vm.count("recv");

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return false;
  }

  if (option->transmit && option->receive) {
    printf("-t -r must be specified.\n");
    return false;
  }

  if (option->transmit) {
    printf("host = %s, port = %d\n", option->host.c_str(), option->port);
    printf("buffer length = %d\n", option->length);
    printf("number of buffers = %d\n", option->number);
  } else if (option->receive) {
    printf("port = %d\n", option->port);
    printf("accepting...\n");
  }
  return true;
}