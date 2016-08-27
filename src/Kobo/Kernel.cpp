/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Kernel.hpp"

#include <stdexcept>

#ifdef KOBO

#include "IO/FileReader.hxx"
#include "IO/GunzipReader.hxx"
#include "IO/BufferedReader.hxx"

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

static bool
Copy(int out_fd, int in_fd, const char *out_path, const char *in_path)
{
  while (true) {
    char buffer[512];
    ssize_t nread = read(in_fd, buffer, sizeof(buffer));
    if (nread < 0) {
      fprintf(stderr, "Failed to read from %s: %s\n",
              in_path, strerror(errno));
      return false;
    }

    if (nread == 0)
      return true;

    ssize_t nwritten = write(out_fd, buffer, nread);
    if (nwritten < 0) {
      fprintf(stderr, "Failed to write to %s: %s\n",
              out_path, strerror(errno));
      return false;
    }

    if (nwritten != nread) {
      fprintf(stderr, "Short write to %s\n", out_path);
      return false;
    }
  }
}

bool
KoboInstallKernel(const char *uimage_path)
{
  const char *const out_path = "/dev/mmcblk0";

  const int in_fd = open(uimage_path, O_RDONLY|O_NOCTTY|O_CLOEXEC);
  if (in_fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", uimage_path, strerror(errno));
    return false;
  }

  const int out_fd = open(out_path, O_WRONLY|O_NOCTTY|O_CLOEXEC);
  if (out_fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", out_path, strerror(errno));
    close(in_fd);
    return false;
  }

  constexpr off_t out_offset = 2048 * 512;

  if (lseek(out_fd, out_offset, SEEK_SET) != out_offset) {
    fprintf(stderr, "Failed to seek %s\n", out_path);
    close(out_fd);
    close(in_fd);
    return false;
  }

  bool success = Copy(out_fd, in_fd, out_path, uimage_path);

  close(in_fd);

  fdatasync(out_fd);
  close(out_fd);

  return success;
}

#endif

bool
IsKoboOTGKernel()
try {
#ifdef KOBO
  FileReader file(Path("/proc/config.gz"));
  GunzipReader gunzip(file);
  BufferedReader reader(gunzip);

  char *line;
  while ((line = reader.ReadLine()) != nullptr)
    if (strcmp(line, "CONFIG_USB_EHCI_ARC_OTG=y") == 0)
      return true;
#endif

  return false;
} catch (const std::runtime_error &e) {
  return false;
}
