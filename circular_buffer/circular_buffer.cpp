#include <stdio.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include <string>

#include "circular_buffer.h"

BOOST_AUTO_TEST_CASE(test_CircularBuffer_push) {
  CircularBuffer circular_buffer;
  BOOST_CHECK_EQUAL(circular_buffer.size(), 0);
  circular_buffer.push(1);
  BOOST_CHECK_EQUAL(circular_buffer.size(), 1);
  circular_buffer.push(1);
  BOOST_CHECK_EQUAL(circular_buffer.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_CircularBuffer_pushmax) {
  CircularBuffer circular_buffer;
  BOOST_CHECK_EQUAL(circular_buffer.size(), 0);

  // max = 1023: size() - 1
  for (size_t i = 0; i < 1023; ++i) {
    circular_buffer.push(1);
  }

  BOOST_CHECK_EQUAL(circular_buffer.size(), 1023);

  BOOST_CHECK_EQUAL(circular_buffer.push(1), 0);
}

BOOST_AUTO_TEST_CASE(test_CircularBuffer_str) {
  CircularBuffer circular_buffer;
  circular_buffer.push(std::string(1023, '1'));
  BOOST_CHECK_EQUAL(circular_buffer.size(), 1023);
}