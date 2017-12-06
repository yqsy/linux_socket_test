#pragma once

#include <iostream>

#include "common.h"

enum RecvState {
  kExpectFrameSize, // Waiting for header 8 Bytes
  kExceptFrame      // Waiting for 4 Bytes length and payload
};

struct EventClient {
  RecvState recv_state;
  int count;
  char *buffer;
  SessionMessage session_message;
  int ack;
  EventClient() {
    recv_state = kExpectFrameSize;
    count = 0;
    buffer = 0;
    ack = 0;
  }
  ~EventClient() { std::cout << "be closed\n"; }
};
