
#include <string>

#include <http/http_context.h>
#include <http/http_request.h>

#include <muduo/net/Buffer.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

using muduo::net::Buffer;

BOOST_AUTO_TEST_CASE(test_http_context_first_line)
{
  HttpContext http_context;

  std::string s = "GET /index.html HTTP/1.1\r\n";
  BOOST_CHECK(
      http_context.parse_first_line(s.c_str(), s.c_str() + s.length() - 2));

  s = "GET/index.htmlHTTP/1.1\r\n";
  BOOST_CHECK(
      !http_context.parse_first_line(s.c_str(), s.c_str() + s.length() - 2));
}

BOOST_AUTO_TEST_CASE(test_http_context_all)
{

  HttpContext http_context;
  Buffer input;
  input.append("GET /index.html HTTP/1.1\r\n"
               "Host: www.chenshuo.com\r\n"
               "\r\n");

  BOOST_CHECK(http_context.parse_request(&input));
  BOOST_CHECK(http_context.got_all());

  http_context.reset();
  BOOST_CHECK(input.readableBytes() == 0);

  input.append("GET /index.html HTTP/1.1\r\n"
               "Host: www.chenshuo.com\r\n"
               "\r");

  BOOST_CHECK(http_context.parse_request(&input));
  BOOST_CHECK(!http_context.got_all());

  input.append("\n");
  BOOST_CHECK(http_context.parse_request(&input));
  BOOST_CHECK(http_context.got_all());
}

BOOST_AUTO_TEST_CASE(test_http_context_all_value)
{
  // 测试每个字段的正确性
}

BOOST_AUTO_TEST_CASE(test_http_context_onebyone)
{
  // 一个一个字节来
}

BOOST_AUTO_TEST_CASE(test_http_context_two_piece)
{
  // 分成两部分
}

BOOST_AUTO_TEST_CASE(test_http_context_empyt_header_value)
{
  // 头部的value为空
}