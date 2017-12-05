// https://embedjournal.com/implementing-circular-buffer-embedded-c/

#pragma once

#include <stdio.h>

#include <algorithm>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/utility/string_ref.hpp>

class CircularBuffer : public boost::noncopyable {
public:
  static const size_t k_init_size = 1024;

  CircularBuffer() : buffer_(k_init_size), read_idx_(0), write_idx_(0) {}

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
    return 1;
  }

  size_t push(boost::string_ref ele) {
    if (free_space_size() < ele.length()) {
      buffer_.resize(ele.length() - free_space_size() + buffer_.size());
    }

    for (size_t i = 0; i < ele.length(); ++i) {
      push(ele[i]);
    }

    return ele.length();
  }

  size_t free_space_size() {
    // One element needs to be occupied so -1
    return buffer_.size() - size() - 1;
  }

  size_t size() {
    if (read_idx_ == write_idx_) {
      return 0;
    } else if (write_idx_ > read_idx_) {
      return write_idx_ - read_idx_;
    } else {
      return write_idx_ + buffer_.size() - read_idx_;
    }
  }

private:
  std::vector<char> buffer_;

  size_t read_idx_;
  size_t write_idx_;
};