/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <algorithm>

/** Create a pseudo TTY, and symlink it to /tmp/nmea. */
static int
open_virtual(const char *symlink_path)
{
  int fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
  if (fd < 0) {
    fprintf(stderr, "failed to open /dev/ptmx: %s\n", strerror(errno));
    _exit(1);
  }

  int ret = unlockpt(fd);
  if (ret < 0) {
    fprintf(stderr, "failed to unlockpt(): %s\n", strerror(errno));
    _exit(1);
  }

  unlink(symlink_path);
  ret = symlink(ptsname(fd), symlink_path);
  if (ret < 0) {
    fprintf(stderr, "symlink() failed: %s\n", strerror(errno));
    _exit(1);
  }

  return fd;
}

int
main(int argc, char **argv)
{
  int fd = open_virtual("/tmp/nmea");

  unsigned long last_stamp = -1;
  char line[1024];
  while (fgets(line, sizeof(line), stdin) != NULL) {
    char *endptr;
    unsigned long current_stamp = strtoul(line, &endptr, 10);
    if (endptr == line || *endptr != ' ' || endptr[1] != '<')
      continue;

    char *start = endptr + 2;
    char *end = strchr(start, '>');
    if (end == NULL)
      continue;

    *end++ = '\n';
    *end = 0;

    if (current_stamp > last_stamp) {
      unsigned long delta_t = std::min(current_stamp - last_stamp, 1000ul);
      usleep(delta_t * 1000);
    }

    last_stamp = current_stamp;

    ssize_t nbytes = write(fd, start, end - start);
    if (nbytes < 0) {
      perror("Failed to write to port\n");
      close(fd);
      return 2;
    }
  }

  close(fd);

  return EXIT_SUCCESS;
}
