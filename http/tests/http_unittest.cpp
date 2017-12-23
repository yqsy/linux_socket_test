
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

BOOST_AUTO_TEST_CASE(test_http_context_body_simple)
{
  // 普通测试请求body
  HttpContext context;
  Buffer input;
  input.append("POST /color.cgi HTTP/1.1\r\n"
               "Host: vm1\r\n"
               "Connection: keep-alive\r\n"
               "Content-Length: 10\r\n"
               "Cache-Control: max-age=0\r\n"
               "Origin: http://vm1\r\n"
               "Upgrade-Insecure-Requests: 1\r\n"
               "Content-Type: application/x-www-form-urlencoded\r\n"
               "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
               "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.84 "
               "Safari/537.36\r\n"
               "Accept: "
               "text/html,application/xhtml+xml,application/xml;q=0.9,image/"
               "webp,image/apng,*/*;q=0.8\r\n"
               "Referer: http://vm1/\r\n"
               "Accept-Encoding: gzip, deflate\r\n"
               "Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,zh-TW;q=0.7\r\n"
               "\r\n"
               "color=blue");
  BOOST_CHECK(context.parse_request(&input));
  BOOST_CHECK(context.got_all());
  const auto &request = context.request();
  BOOST_CHECK(context.got_all());
  BOOST_CHECK_EQUAL(request.body().size(), 10);
  BOOST_CHECK_EQUAL(request.get_remain_body_len(), 0);
  BOOST_CHECK_EQUAL(request.body(), string("color=blue"));
}

BOOST_AUTO_TEST_CASE(test_http_context_body_one_by_one)
{
  // body 一个一个字节解析
  HttpContext context;
  Buffer input;
  input.append("POST /color.cgi HTTP/1.1\r\n"
               "Connection: keep-alive\r\n"
               "Content-Length: 10\r\n"
               "\r\n");
  BOOST_CHECK(context.parse_request(&input));
  BOOST_CHECK(!context.got_all());

  string tmp = "color=blue";

  for (auto &v : tmp)
  {
    input.append(string(1, v));
    BOOST_CHECK(context.parse_request(&input));
  }

  BOOST_CHECK(context.got_all());
  const auto &request = context.request();
  BOOST_CHECK_EQUAL(request.body().size(), 10);
  BOOST_CHECK_EQUAL(request.get_remain_body_len(), 0);
  BOOST_CHECK_EQUAL(request.body(), string("color=blue"));
}

BOOST_AUTO_TEST_CASE(test_http_context_body_more)
{
  // body没到齐,和下一个请求粘在一起了,不过应该不太可能!
  HttpContext context;
  Buffer input;
  input.append("POST /color.cgi HTTP/1.1\r\n"
               "Connection: keep-alive\r\n"
               "Content-Length: 10\r\n"
               "\r\n"
               "color=");
  BOOST_CHECK(context.parse_request(&input));
  BOOST_CHECK(!context.got_all());

  BOOST_CHECK_EQUAL(input.readableBytes(), 0);

  string next_str = string("blue"
                           "POST /color.cgi HTTP/1.1\r\n"
                           "Connection: keep-alive\r\n"
                           "Content-Length: 10\r\n"
                           "\r\n"
                           "color=blue\r\n");
  input.append(next_str);

  BOOST_CHECK(context.parse_request(&input));
  BOOST_CHECK(context.got_all());

  // parsed 4 bytes
  BOOST_CHECK_EQUAL(input.readableBytes(), next_str.size() - 4);

  // 还是kGotAll,除非你reset了,才能解析下一个context
  BOOST_CHECK(context.parse_request(&input));
  BOOST_CHECK_EQUAL(input.readableBytes(), next_str.size() - 4);
  const auto &request = context.request();
  BOOST_CHECK_EQUAL(request.body(), string("color=blue"));

  // 解析剩余的下一个串
  context.reset();
  BOOST_CHECK(context.parse_request(&input));
  BOOST_CHECK(context.got_all());
  const auto &request2 = context.request();
  BOOST_CHECK_EQUAL(input.readableBytes(), 2);
  BOOST_CHECK_EQUAL(request2.body().size(), 10);
  BOOST_CHECK_EQUAL(request2.get_remain_body_len(), 0);
  BOOST_CHECK_EQUAL(request2.body(), string("color=blue"));

  // 剩余的\r\n
  context.reset();
  BOOST_CHECK(!context.parse_request(&input));
}