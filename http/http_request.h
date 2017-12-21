#pragma once

#include <muduo/base/Types.h>

using namespace muduo;

class HttpRequest
{
public:
  enum Method
  {
    kInvalid,
    kGet,
    kPost,
    kHead,
    kPut,
    kDelete
  };

  enum Version
  {
    kUnKnown,
    kHttp10,
    kHttp11
  };

  HttpRequest() : method_(kInvalid), version_(kUnKnown) {}

  bool set_method(const char *start, const char *end)
  {
    string m(start, end);
    if (m == "GET")
    {
      method_ = kGet;
    }
    else if (m == "POST")
    {
      method_ = kPost;
    }
    else if (m == "HEAD")
    {
      method_ = kHead;
    }
    else if (m == "PUT")
    {
      method_ = kPut;
    }
    else if (m == "DELETE")
    {
      method_ = kDelete;
    }
    else
    {
      method_ = kInvalid;
    }

    return method_ != kInvalid;
  }

  void set_version(Version v) { version_ = v; }

  void set_path(const char *start, const char *end)
  {
    path_.assign(start, end);
  }

  void set_query(const char *start, const char *end)
  {
    query_.assign(start, end);
  }

private:
  Method method_;
  string path_;
  string query_;
  Version version_;
};