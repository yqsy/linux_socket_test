#pragma once

#include <arpa/inet.h> // uint16_t

#include <string>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

enum Command
{
  k_server,
  k_client,
  k_none
};

extern const int MAX_BUFFER_LENGTH;

struct Options
{
  // option
  Command command;

  // server
  // std::string listen_host;
  uint16_t listen_port;

  // client
  std::string server_host;
  uint16_t server_port;
  int number;
  int length;
  Options() : command(k_none), number(0), length(0) {}
};

struct SessionMessage
{
  int32_t number;
  int32_t length;
} __attribute__((__packed__));

struct PayloadMessage
{
  int32_t length;
  char data[0];
};

bool parse_commandline(int argc, char *argv[], Options *opt);

struct sockaddr_in resolve_or_die(const char *host, uint16_t port);