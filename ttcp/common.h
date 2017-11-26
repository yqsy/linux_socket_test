#pragma once

int write_n(int sockfd, const void *buf, int length);

int read_n(int sockfd, void *buf, int length);

#include <stdint.h>

struct SessionMessage {
  int32_t number;
  int32_t length;
} __attribute__((__packed__));

struct PayloadMessage {
  int32_t length;
  char data[0];
};
