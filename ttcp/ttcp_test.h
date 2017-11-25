#pragma once

#include <stdint.h>

struct SessionMessage {
  int32_t number;
  int32_t length;
} __attribute__((__packed__));

struct PayloadMessage {
  int32_t length;
  char data[0];
};