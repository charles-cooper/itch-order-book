#include <errno.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include "bufferedreader.h"

/** Blocking call. Ensures that n bytes are available to consume.
 * If they are there already it will return immediately. Else
 * it will attempt to read. If it is able to read enough bytes
 * then it returns true. On read error it will return false.
 */
read_t buf_t::ensure(unsigned const n)
{
  if (this->available(n))
    return read_t::OK;  // fast path. call before read for performance

  ssize_t bytes = this->read(n);
  if (bytes == 0)
    return read_t::ERR;  // EOF
  else if (bytes < 0)
    return read_t::ERR;  // read error
  else
    return read_t::OK;
}

/** Blocking read.
 * If n > read_max_bufpos - read_bufpos, it will read into the buffer.
 * If n > buflen - read_bufpos it will also
 * If n > buflen and not compiled with -DNDEBUG it will assert false.
 * If n > buflen and compiled with -DNDEBUG the behavior is undefined.
 * This will not advance pos but may advance limit
 * Returns 0 on EOF and a negative number on read error
 */
ssize_t buf_t::read(unsigned const n)
{
  assert(n <= len);
  assert(pos <= limit);
  assert(limit <= len);
  if (pos + n > len) discard_to_pos();

  ssize_t bytes_read = 0;
  while (this->available() < n) {
    ssize_t bytes = this->read();
    if (bytes < 0) {
      perror("Read error:");
      return bytes;
    }
    if (0 == bytes) return 0;  // EOF and this must be handled.
    bytes_read += bytes;
  }
  return bytes_read;
}

// reads as many bytes as possible into buf and returns num bytes
// advances read_max_bufpos and read_bufpos
ssize_t buf_t::read()
{
  // read into offset however many free bytes left in buffer
  ssize_t bytes = ::read(this->fd, (void*)(ptr + limit), len - limit);
  if (-1 == bytes) {
    perror("blocking read into buf!");
    return -1;
  }
  assert(0 <= bytes);
  assert(bytes <= len);
  limit += (unsigned)bytes;
  bytesread += (size_t)bytes;
  assert(pos <= limit);
  assert(limit <= len);
  return bytes;
}

/* behavior is almost identical to nb_read_into_buf but it returns -1 on any
 * error
 * and only returns zero in the same circumstances that recv or read would (i.e.
 * EOF)*/
ssize_t buf_t::nb_read()
{
  ssize_t bytes =
      recv(this->fd, (void*)(ptr + limit), len - limit, MSG_DONTWAIT);
  if (-1 == bytes) return -1;
  assert(bytes <= len);
  assert(bytes >= 0);
  limit += (unsigned)bytes;
  bytesread += (size_t)bytes;
  assert(pos <= limit);
  assert(limit <= len);
  return bytes;
}
