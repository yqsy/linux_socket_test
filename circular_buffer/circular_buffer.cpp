#include <circular_buffer/circular_buffer.h>

#include <errno.h>

ssize_t CircularBuffer::readfd(int fd /*, int *saved_errno*/)
{
  char extrabuf[65536];
  ssize_t nr = read(fd, extrabuf, sizeof(extrabuf));
  if (nr < 0)
  {
    // *saved_errno = errno;
  }
  else if (nr == 0)
  {
  }
  else
  {
    push_str(boost::string_ref(extrabuf, nr));
  }
  return nr;
}
