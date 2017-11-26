#pragma once

#include <ev.h>

#include <memory>
#include <vector>

struct EvClient {
  ev_io io;
  int fd;
};

typedef std::shared_ptr<EvClient> EvClientPtr;
