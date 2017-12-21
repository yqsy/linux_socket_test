
#include <string>

#include <http/http_context.h>
#include <http/http_request.h>

#include <muduo/net/Buffer.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_http_context)
{
  HttpContext http_context;

  std::string s = "GET /index.html HTTP/1.1\r\n";

  BOOST_CHECK(
      http_context.parse_first_line(s.c_str(), s.c_str() + s.length() - 2));
}