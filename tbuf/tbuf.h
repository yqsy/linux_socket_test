#pragma once

#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>

#include <vector>

#include <boost/implicit_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/utility/string_ref.hpp>

class Tbuf : public boost::noncopyable {
public:
  static const int k_init_size = 1024;

  Tbuf() : ridx_(0), widx_(0), buf_(k_init_size) {}

  size_t readable_bytes() const { return widx_ - ridx_; }

  size_t writeable_bytes() const { return buf_.size() - widx_; }

  size_t prependable_bytes() const { return ridx_; }

  char *cur_write_p() { return &*buf_.begin() + widx_; }

  const char *end_write_p() const { return &*buf_.begin() + buf_.size(); }

  char *begin() { return &*buf_.begin(); };

  const char *begin() const { return &*buf_.begin(); };

  void append(boost::string_ref str) {
    if (str.size() > writeable_bytes()) {
      if (str.size() <= writeable_bytes() + prependable_bytes()) {
        // transfer
        size_t transfer_size = readable_bytes();
        std::copy(begin() + ridx_, begin() + widx_, begin());
        ridx_ = 0;
        widx_ = 0 + transfer_size;
      } else {
        // resize

        // capacity grows exponentiallyexponentially, possible to not allocate
        // new memory
        buf_.resize(widx_ + str.size());
      }

      assert(writeable_bytes() >= str.size());
    }

    assert(end_write_p() == &*buf_.end() && cur_write_p() < end_write_p());

    std::copy(str.begin(), str.end(), cur_write_p());
    widx_ += str.size();
  }

  const char *peek() const { return begin() + ridx_; }

  int32_t peek_int32() const {
    int32_t rtn;
    memcpy(&rtn, peek(), sizeof(rtn));
    return ntohl(rtn);
  }

  int32_t read_int32() {
    int32_t rtn = peek_int32();
    retrieve(sizeof(rtn));
    return rtn;
  }

  void retrieve(size_t len) {
    assert(len <= readable_bytes());
    if (len < readable_bytes()) {
      ridx_ += len;
    } else {
      ridx_ = 0;
      widx_ = 0;
    }
  }

  // >0 readed bytes
  // =0 need to be close???
  // <0 error
  ssize_t readfd(int fd) {

    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writeable = writeable_bytes();

    vec[0].iov_base = begin() + widx_;
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writeable < sizeof(extrabuf)) ? 2 : 1;

    const ssize_t n = readv(fd, vec, iovcnt);
    if (n < 0) {
      // errno
    } else if (boost::implicit_cast<size_t>(n) <= writeable) {
      widx_ += n;
    } else {
      widx_ = buf_.size();
      append(boost::string_ref(extrabuf, n - writeable));
    }
    return n;
  }

private:
  size_t ridx_;
  size_t widx_;
  std::vector<char> buf_;
};
