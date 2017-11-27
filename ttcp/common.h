#pragma once

#include <stdint.h>
#include <string>

struct SessionMessage {
  int32_t number;
  int32_t length;
} __attribute__((__packed__));

struct PayloadMessage {
  int32_t length;
  char data[0];
};

struct Option {
  uint16_t port;
  int length;
  int number;
  std::string host;
  bool transmit, receive;
};

bool parse_command_line(int argc, char *argv[], Option *option);