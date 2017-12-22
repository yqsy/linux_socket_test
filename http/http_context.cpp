#include <http/http_context.h>

#include <muduo/base/Logging.h>

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
          // empty line
          state_ = kGotAll;
          buf->retrieveUntil(crlf + 2);
          break;
        }
      }
      else
      {
        break;
      }
    }
    else if (state_ == kExpectBody)
    {
      // TODO
    }
    else // kGotAll
    {
      LOG_WARN << "kGotAll";
      break;
    }
  }

  return true;
}
