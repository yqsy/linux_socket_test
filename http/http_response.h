#pragma once

#include <map>
#include <string>

#include <muduo/base/Types.h>
#include <muduo/net/Buffer.h>

using namespace muduo;
using namespace muduo::net;

class HttpResponse
{
public:
  enum HttpStatusCode
  {
    kUnKnown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
    k501NotImplemented = 501,
    k500Error = 500
  };

  HttpResponse(bool close) : status_code_(kUnKnown), close_connection_(close) {}

  void add_header(const std::string &key, const std::string &value)
  {
    headers_[key] = value;
  }

  void set_content_type(const std::string &content_type)
  {
    add_header("Content-Type", content_type);
  }

  void set_status_message(const std::string &message)
  {
    status_message_ = message;
  }

  void set_status_code(HttpStatusCode code) { status_code_ = code; }

  void set_close_connection(bool on) { close_connection_ = on; }

  void set_body(const std::string &body) { body_ = body; }

  void append_body(const std::string &body) { body_.append(body); }

  bool close_connection() const { return close_connection_; }

  void append_to_buffer(Buffer *output);

private:
  std::map<std::string, std::string> headers_;
  std::string body_;
  std::string status_message_;
  HttpStatusCode status_code_;
  bool close_connection_;
};