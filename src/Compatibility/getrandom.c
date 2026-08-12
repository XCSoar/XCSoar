// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

ssize_t getrandom(void *buffer, size_t count, unsigned int flags);

ssize_t
getrandom(void *buffer, size_t count, unsigned int flags)
{
  (void)flags;

  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0)
    return -1;

  char *p = buffer;
  size_t remaining = count;
  while (remaining > 0) {
    ssize_t n = read(fd, p, remaining);
    if (n < 0) {
      int saved_errno = errno;
      close(fd);
      errno = saved_errno;
      return -1;
    }

    if (n == 0) {
      close(fd);
      errno = EIO;
      return -1;
    }

    p += n;
    remaining -= n;
  }

  close(fd);
  return count;
}
