#pragma once

#include <map>

#include <boost/lexical_cast.hpp>

#include <muduo/base/Types.h>

using namespace muduo;

using boost::bad_lexical_cast;
using boost::lexical_cast;

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

  Method method() const { return method_; }

  string method_str() const
  {
    if (method_ == kGet)
    {
      return "GET";
    }
    else if (method_ == kPost)
    {
      return "POST";
    }
    else if (method_ == kHead)
    {
      return "HEAD";
    }
    else if (method_ == kPut)
    {
      return "PUT";
    }
    else if (method_ == kDelete)
    {
      return "DELETE";
    }

    return "INVALID";
  }

  void set_version(Version v) { version_ = v; }

  Version version() const { return version_; }

  void set_path(const char *start, const char *end)
  {
    path_.assign(start, end);
  }

  const string &path() const { return path_; }

  void set_query(const char *start, const char *end)
  {
    query_.assign(start, end);
  }

  const string &query() const { return query_; }

  void add_header(const char *start, const char *colon, const char *end)
  {
    string field(start, colon);

    colon++;
    while (colon < end && isspace(*colon))
    {
      ++colon;
    }

    string value(colon, end);
    while (!value.empty() && isspace(value[value.size() - 1]))
    {
      value.resize(value.size() - 1);
    }
    headers_[field] = value;
  }

  string get_header(const string &field) const
  {
    string result;
    auto it = headers_.find(field);
    if (it != headers_.end())
    {
      result = it->second;
    }
    return result;
  }

  ssize_t get_expect_body_len() const
  {
    ssize_t expect_body_len = 0;

    try
    {
      expect_body_len = lexical_cast<ssize_t>(get_header("Content-Length"));
    }
    catch (bad_lexical_cast &)
    {
      //
      expect_body_len = 0;
    }

    return expect_body_len;
  }

  ssize_t get_remain_body_len() const
  {
    ssize_t expect_body_len = get_expect_body_len();
    ssize_t current_body_len = body().size();

    assert(expect_body_len >= current_body_len);
    return expect_body_len - current_body_len;
  }

  void append_body(const string &body) { body_.append(body); }

  const string &body() const { return body_; }

private:
  Method method_;
  string path_;
  string query_;
  string body_;
  Version version_;
  std::map<string, string> headers_;
};
