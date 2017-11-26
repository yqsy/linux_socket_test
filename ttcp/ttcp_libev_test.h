#pragma once

#include <ev.h>

#include <memory>
#include <vector>

// FIXME:
#include "common.h"

struct EvClient;
typedef std::shared_ptr<EvClient> EvClientPtr;

struct EvServer;

struct EvClient {
  ev_io io;
  int fd;
  SessionMessage session_message;
  char *buffer;
  int ack;
  int count;
  EvServer *ev_server;
};

struct EvServer {
  ev_io io;
  int fd;
  std::vector<EvClientPtr> ev_clients;
};
