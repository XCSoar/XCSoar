/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

/*
 * This program creates a pseudo-TTY symlinked to /tmp/nmea, and feeds
 * NMEA data read from stdin to it.  It is useful to feed WINE with
 * it: symlink ~/.wine/dosdevices/com1 to /tmp/nmea, and configure
 * "COM1" in XCSoar.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/**
 * Create a pseudo TTY, and symlink it to /tmp/nmea.
 */
static int
open_virtual(const char *symlink_path)
{
    int fd, ret;

    fd = open("/dev/ptmx", O_RDWR|O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "failed to open /dev/ptmx: %s\n",
                strerror(errno));
        _exit(1);
    }

    ret = unlockpt(fd);
    if (ret < 0) {
        fprintf(stderr, "failed to unlockpt(): %s\n",
                strerror(errno));
        _exit(1);
    }

    unlink(symlink_path);
    ret = symlink(ptsname(fd), symlink_path);
    if (ret < 0) {
        fprintf(stderr, "symlink() failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    return fd;
}

int main(int argc, char **argv)
{
  int fd = open_virtual("/tmp/nmea");

  char stamp[6] = "";

  char line[1024];
  while (fgets(line, sizeof(line), stdin) != NULL) {
    if (memcmp(line, "$GP", 3) == 0 &&
        (memcmp(line + 3, "GGA", 3) == 0 ||
         memcmp(line + 3, "RMC", 3) == 0) &&
        line[6] == ',' &&
        strncmp(stamp, line + 7, sizeof(stamp)) != 0) {
      /* the time stamp has changed - sleep for one second */
      sleep(1);
      strncpy(stamp, line + 7, sizeof(stamp));
    }

    size_t length = strlen(line);
    ssize_t nbytes = write(fd, line, length);
    if (nbytes < 0) {
      perror("Failed to write to port\n");
      close(fd);
      return 2;
    }
  }

  close(fd);

  return EXIT_SUCCESS;
}
