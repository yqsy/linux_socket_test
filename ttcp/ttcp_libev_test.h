#pragma once

#include <ev.h>

#include <memory>
#include <vector>

// FIXME:
#include "common.h"

struct EvClient;
struct EvServer;

typedef std::shared_ptr<EvClient> EvClientPtr;

enum RecvState {
  kExpectFrameSize, // Waiting for header 8 Bytes
  kExceptFrame      // Waiting for 4 Bytes length and payload
};

struct EvClient {
  ev_io io;
  int fd;
  SessionMessage session_message;
  char *buffer;
  int ack;
  int count;
  RecvState recv_state;
  EvServer *ev_server;
};

struct EvServer {
  ev_io io;
  int fd;
  std::vector<EvClientPtr> ev_clients;
};
