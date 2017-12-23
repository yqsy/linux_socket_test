#include <http/http_context.h>

#include <boost/lexical_cast.hpp>

#include <muduo/base/Logging.h>

using boost::bad_lexical_cast;
using boost::lexical_cast;

bool HttpContext::parse_first_line(const char *begin, const char *end)
{
  const char *start = begin;
  const char *space = std::find(start, end, ' ');
  // find method
  if (space == end)
  {
    return false;
  }

  if (!request_.set_method(start, space))
  {
    return false;
  }

  // find path and query
  start = space + 1;
  space = std::find(start, end, ' ');
  if (space == end)
  {
    return false;
  }

  const char *question = std::find(start, space, '?');
  if (question != space)
  {
    request_.set_path(start, question);
    request_.set_query(question, space);
  }
  else
  {
    request_.set_path(start, space);
  }

  // find version
  start = space + 1;
  bool v = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
  if (!v)
  {
    return false;
  }

  if (*(end - 1) == '1')
  {
    request_.set_version(HttpRequest::kHttp11);
  }
  else if (*(end - 1) == '0')
  {
    request_.set_version(HttpRequest::kHttp10);
  }
  else
  {
    return false;
  }

  return true;
}

bool HttpContext::parse_request(muduo::net::Buffer *buf)
{
  while (true)
  {
    if (state_ == kExpectRequestLine)
    {
      const char *crlf = buf->findCRLF();

      if (crlf)
      {
        if (parse_first_line(buf->peek(), crlf))
        {
          buf->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        }
        else
        {
          // parse first line error
          return false;
        }
      }
      else
      {
        break;
      }
    }
    else if (state_ == kExpectHeaders)
    {
      const char *crlf = buf->findCRLF();

      if (crlf)
      {
        const char *colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          request_.add_header(buf->peek(), colon, crlf);
          buf->retrieveUntil(crlf + 2);
        }
        else
        {
          if (request_.get_header("Content-Length") != "")
          {
            state_ = kExpectBody;
            buf->retrieveUntil(crlf + 2);
          }
          else
          {
            // empty line
            state_ = kGotAll;
            buf->retrieveUntil(crlf + 2);
            break;
          }
        }
      }
      else
      {
        break;
      }
    }
    else if (state_ == kExpectBody)
    {
      // 剩余需要的body字节数
      ssize_t remain_body_len = request_.get_remain_body_len();

      if (static_cast<ssize_t>(buf->readableBytes()) > remain_body_len)
      {
        request_.append_body(string(buf->peek(), remain_body_len));
        buf->retrieve(remain_body_len);

        assert(request_.get_remain_body_len() == 0);
      }
      else
      {
        request_.append_body(string(buf->peek(), buf->readableBytes()));
        buf->retrieveAll();
      }

      if (request_.get_remain_body_len() == 0)
      {
        state_ = kGotAll;
      }
      break;
    }
    else // kGotAll
    {
      LOG_WARN << "kGotAll";
      break;
    }
  }

  return true;
}
