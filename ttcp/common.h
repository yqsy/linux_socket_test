#pragma once

#include <arpa/inet.h> // uint16_t

#include <string>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

enum Command { k_server, k_client, k_none };

struct Options {
  // option
  Command command;

  // server
  std::string listen_host;
  uint16_t listen_port;

  // client
  std::string server_host;
  uint16_t server_port;
  int buffer_number;
  int buffer_length;
  Options() : command(k_none), buffer_number(0), buffer_length(0) {}
};

bool parse_commandline(int argc, char *argv[], Options *opt);