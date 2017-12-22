
#include <string>

#include <http/http_context.h>
#include <http/http_request.h>

#include <muduo/base/Types.h>
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
  HttpContext context;
  Buffer input;
  input.append("GET /index.html HTTP/1.1\r\n"
               "Host: www.chenshuo.com\r\n"
               "\r\n");
  BOOST_CHECK(context.parse_request(&input));
  BOOST_CHECK(context.got_all());
  const auto &request = context.request();
  BOOST_CHECK_EQUAL(request.method(), HttpRequest::kGet);
  BOOST_CHECK_EQUAL(request.path(), string("/index.html"));
  BOOST_CHECK_EQUAL(request.version(), HttpRequest::kHttp11);
  BOOST_CHECK_EQUAL(request.get_header("Host"), string("www.chenshuo.com"));
  BOOST_CHECK_EQUAL(request.get_header("User-Agent"), string(""));
}

BOOST_AUTO_TEST_CASE(test_http_context_onebyone)
{
  // 一个一个字节来
  string all("GET /index.html HTTP/1.1\r\n"
             "Host: www.chenshuo.com\r\n"
             "\r\n");
  Buffer input;
  HttpContext context;
  for (auto &s : all)
  {
    input.append(string(1, s));
    BOOST_CHECK(context.parse_request(&input));
  }
  BOOST_CHECK(context.got_all());
  const auto &request = context.request();
  BOOST_CHECK_EQUAL(request.method(), HttpRequest::kGet);
  BOOST_CHECK_EQUAL(request.path(), string("/index.html"));
  BOOST_CHECK_EQUAL(request.version(), HttpRequest::kHttp11);
  BOOST_CHECK_EQUAL(request.get_header("Host"), string("www.chenshuo.com"));
  BOOST_CHECK_EQUAL(request.get_header("User-Agent"), string(""));
}

BOOST_AUTO_TEST_CASE(test_http_context_two_piece)
{
  // 分成两部分
  string all("GET /index.html HTTP/1.1\r\n"
             "Host: www.chenshuo.com\r\n"
             "\r\n");

  for (size_t sz1 = 0; sz1 < all.size(); ++sz1)
  {
    Buffer input;
    HttpContext context;
    input.append(all.c_str(), sz1);
    BOOST_CHECK(context.parse_request(&input));

    input.append(all.c_str() + sz1, all.size() - sz1);
    BOOST_CHECK(context.parse_request(&input));
    BOOST_CHECK(context.got_all());
    const auto &request = context.request();
    BOOST_CHECK_EQUAL(request.method(), HttpRequest::kGet);
    BOOST_CHECK_EQUAL(request.path(), string("/index.html"));
    BOOST_CHECK_EQUAL(request.version(), HttpRequest::kHttp11);
    BOOST_CHECK_EQUAL(request.get_header("Host"), string("www.chenshuo.com"));
    BOOST_CHECK_EQUAL(request.get_header("User-Agent"), string(""));
  }
}

BOOST_AUTO_TEST_CASE(test_http_context_empyt_header_value)
{
  // 头部的value为空

  HttpContext context;
  Buffer input;
  input.append("GET /index.html HTTP/1.1\r\n"
               "Host: www.chenshuo.com\r\n"
               "User-Agent:\r\n"
               "Accept-Encoding: \r\n"
               "\r\n");
  BOOST_CHECK(context.parse_request(&input));
  BOOST_CHECK(context.got_all());

  const auto &request = context.request();
  BOOST_CHECK_EQUAL(request.method(), HttpRequest::kGet);
  BOOST_CHECK_EQUAL(request.path(), string("/index.html"));
  BOOST_CHECK_EQUAL(request.version(), HttpRequest::kHttp11);
  BOOST_CHECK_EQUAL(request.get_header("Host"), string("www.chenshuo.com"));
  BOOST_CHECK_EQUAL(request.get_header("User-Agent"), string(""));
  BOOST_CHECK_EQUAL(request.get_header("Accept-Encoding"), string(""));
}