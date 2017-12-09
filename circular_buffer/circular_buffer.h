// https://embedjournal.com/implementing-circular-buffer-embedded-c/

#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/utility/string_ref.hpp>

class CircularBuffer : public boost::noncopyable {
public:
  static const size_t k_init_size = 1024;

  CircularBuffer()
      : buffer_(k_init_size), readable_bytes_(0), read_idx_(0), write_idx_(0) {}

  size_t push(const char ele) {
    size_t next = write_idx_ + 1;
    if (next >= buffer_.size()) {
      next = 0;
    }

    if (next == read_idx_) {
      return 0;
    }

    buffer_[write_idx_] = ele;
    write_idx_ = next;

    readable_bytes_++;
    return 1;
  }

  // One ele
  size_t pop(char *ele) {
    if (read_idx_ == write_idx_) {
      return 0;
    }

    size_t next = read_idx_ + 1;
    if (next >= buffer_.size()) {
      next = 0;
    }

    if (ele != nullptr) {
      *ele = buffer_[read_idx_];
    }
    read_idx_ = next;

    readable_bytes_--;
    return 1;
  }

  size_t push_str(boost::string_ref ele) {
    if (writeable_bytes() < ele.length()) {
      size_t old_buffer_size = buffer_.size();
      size_t make_size = ele.length() - writeable_bytes() + buffer_.size();

      if (write_idx_ < read_idx_) {
        std::vector<char> new_buffer(make_size);

        std::copy(std::next(buffer_.begin(), read_idx_), buffer_.end(),
                  new_buffer.begin());
        std::copy(buffer_.begin(), std::next(buffer_.begin(), write_idx_),
                  std::next(new_buffer.begin(), old_buffer_size - read_idx_));

        write_idx_ = old_buffer_size - read_idx_ + write_idx_;
        read_idx_ = 0;

        new_buffer.swap(buffer_);
      } else {
        buffer_.resize(make_size);
        assert(buffer_.size() == make_size);
      }
    }

    assert(writeable_bytes() >= ele.length());

    for (size_t i = 0; i < ele.length(); ++i) {
      int rtn = push(ele[i]);
      assert(rtn != 0);
    }

    return ele.length();
  }

  int32_t read_int32() {
    assert(readable_bytes() >= sizeof(int32_t));
    int32_t out = 0;
    char *pout = reinterpret_cast<char *>(&out);
    for (size_t i = 0; i < sizeof(out); ++i) {
      int rtn = pop(&pout[i]);
      assert(rtn != 0);
    }
    out = ntohl(out);
    return out;
  }

  void retrieve(size_t len) {
    if (len <= readable_bytes()) {
      for (size_t i = 0; i < len; ++i) {
        int rtn = pop(nullptr);
        assert(rtn != 0);
      }

    } else {
      assert(0);
    }
  }

  size_t writeable_bytes() {
    // One element needs to be occupied so -1
    return buffer_.size() - readable_bytes() - 1;
  }

  size_t readable_bytes() {
    // ERROR! when buffer.size() increase
    // if (read_idx_ == write_idx_) {
    //   return 0;
    // } else if (write_idx_ > read_idx_) {
    //   return write_idx_ - read_idx_;
    // } else {
    //   // ERROR: buffer.size increase
    //   return buffer_.size() - (read_idx_ - write_idx_);
    // }
    return readable_bytes_;
  }

  ssize_t readfd(int fd /*, int *saved_errno*/);

private:
  std::vector<char> buffer_;

  size_t readable_bytes_;

public:
  // for unit test
  size_t read_idx_;
  size_t write_idx_;
};