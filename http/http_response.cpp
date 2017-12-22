#include <http/http_response.h>

void HttpResponse::append_to_buffer(Buffer *output)
{
  char buf[32];
  snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", status_code_);
  output->append(buf);
  output->append(status_message_);
  output->append("\r\n");

  if (close_connection_)
  {
    output->append("Connection: close\r\n");
  }
  else
  {
    snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", body_.size());
    output->append(buf);
    output->append("Connection: Keep-Alive\r\n");
  }

  for (auto it = headers_.begin(); it != headers_.end(); ++it)
  {
    output->append(it->first);
    output->append(": ");
    output->append(it->second);
    output->append("\r\n");
  }

  output->append("\r\n");
  output->append(body_);
}