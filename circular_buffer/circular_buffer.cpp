#include <stdio.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "circular_buffer.h"

BOOST_AUTO_TEST_CASE(test_CircularBuffer_push) {
  CircularBuffer circular_buffer;
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 0);
  circular_buffer.push(1);
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 1);
  circular_buffer.push(1);
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 2);
}

BOOST_AUTO_TEST_CASE(test_CircularBuffer_pushmax) {
  CircularBuffer circular_buffer;
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 0);

  // max = 1023: size() - 1
  for (size_t i = 0; i < 1023; ++i) {
    circular_buffer.push(1);
  }

  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 1023);

  BOOST_CHECK_EQUAL(circular_buffer.push(1), 0);
}

BOOST_AUTO_TEST_CASE(test_CircularBuffer_str) {
  CircularBuffer circular_buffer;
  circular_buffer.push_str(std::string(1023, '1'));
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 1023);

  circular_buffer.push_str(std::string(1023, '1'));
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 2 * 1023);

  circular_buffer.push_str(std::string(1023, '1'));
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 3 * 1023);
}

BOOST_AUTO_TEST_CASE(test_CircularBuffer_pop) {
  CircularBuffer circular_buffer;
  circular_buffer.push(1);
  circular_buffer.push(2);
  circular_buffer.pop(nullptr);
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 1);
}

BOOST_AUTO_TEST_CASE(test_CircularBuffer_writeable_bytes) {
  CircularBuffer circular_buffer;
  // default 1024
  BOOST_CHECK_EQUAL(circular_buffer.writeable_bytes(), 1023);
}

BOOST_AUTO_TEST_CASE(test_CircularBuffer_retire) {
  CircularBuffer circular_buffer;
  circular_buffer.push_str(std::string(1023, '1'));

  circular_buffer.retrieve(circular_buffer.readable_bytes());
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 0);
  BOOST_CHECK_EQUAL(circular_buffer.writeable_bytes(), 1023);

  circular_buffer.push_str(std::string(1023, '1'));
  circular_buffer.push_str(std::string(1023, '1'));
  circular_buffer.push_str(std::string(1023, '1'));
  circular_buffer.push_str(std::string(1023, '1'));
  circular_buffer.retrieve(1023 * 2);
  BOOST_CHECK_EQUAL(circular_buffer.readable_bytes(), 2 * 1023);
  BOOST_CHECK_EQUAL(circular_buffer.writeable_bytes(), 2 * 1023);
}

BOOST_AUTO_TEST_CASE(test_stdcopy_copy) {

  const char *buffer = "123456";
  auto begin = &buffer[0];
  auto end = &buffer[0] + 3;
  std::vector<char> outbuffer(std::distance(begin, end));
  std::copy(begin, end, outbuffer.begin());

  BOOST_CHECK_EQUAL_COLLECTIONS(begin, end, outbuffer.begin(), outbuffer.end());
}

BOOST_AUTO_TEST_CASE(test_string_ref_length) {
  auto len = boost::string_ref("123456").length();
  BOOST_CHECK_EQUAL(len, 6);
}
