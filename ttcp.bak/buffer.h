#pragma once

#include <string.h>
#include <sys/uio.h>

#include <vector>

#include <boost/noncopyable.hpp>

class Buffer : public boost::noncopyable {
public:
  static const size_t k_initial_size = 1024;

  explicit Buffer(size_t initial_size = k_initial_size)
      : buffer_(initial_size), read_index_(0), write_index_(0) {}

  size_t readable_bytes() { return write_index_ - read_index_; }

  size_t writeable_bytes() { return buffer_.size() - write_index_; }

  char *begin() { return &*buffer_.begin(); }

  char *begin_write() { return &*buffer_.begin() + write_index_; };

  ssize_t readfd(int fd) {
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writeable = writeable_bytes();
    vec[0].iov_base = begin_write();
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    ssize_t n = readv(fd, vec, 2);
    // assert(n > 0);

    // // read first buf
    // if (n <= writeable) {

    // } else {
    //   // read first and extra buf
    // }
  }

  void ensure_writeable_bytes(size_t len) {}

private:
  std::vector<char> buffer_;

  size_t read_index_;
  size_t write_index_;
};