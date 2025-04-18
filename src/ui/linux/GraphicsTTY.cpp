// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GraphicsTTY.hpp"

#include <errno.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

LinuxGraphicsTTY::LinuxGraphicsTTY() noexcept
  :fd(open(path, O_RDWR | O_NOCTTY | O_CLOEXEC))
{
  if (fd < 0) {
    fprintf(stderr, "Warning: failed to open %s: %s\n",
            path, strerror(errno));
    return;
  }

  if (ioctl(fd, KDSETMODE, KD_GRAPHICS) < 0)
    fprintf(stderr, "Warning: failed to set graphics mode on %s: %s\n",
            path, strerror(errno));
}

LinuxGraphicsTTY::~LinuxGraphicsTTY() noexcept
{
  if (fd >= 0) {
    ioctl(fd, KDSETMODE, KD_TEXT);
    close(fd);
  }
}
