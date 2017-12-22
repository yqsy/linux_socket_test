#pragma once

#include <muduo/net/Buffer.h>

#include <http/http_request.h>

class HttpContext
{
public:
  enum HttpRequestParseState
  {
    kExpectRequestLine,
    kExpectHeaders,
    kExpectBody,
    kGotAll
  };

  HttpContext() : state_(kExpectRequestLine) {}

  bool parse_request(muduo::net::Buffer *buf);

  // method path query version
  bool parse_first_line(const char *begin, const char *end);

  bool got_all() { return state_ == kGotAll; }

  void reset()
  {
    state_ = kExpectRequestLine;
    HttpRequest request;
    request_ = request;
  }

private:
  HttpRequestParseState state_;
  HttpRequest request_;
};
