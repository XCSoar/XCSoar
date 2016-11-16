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

#include "Model.hpp"

#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

static bool
ReadFromFile(const char *path, off_t offset, void *buffer, size_t size)
{
  const int fd = open(path, O_RDONLY|O_NOCTTY|O_CLOEXEC);
  if (fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
    return false;
  }

  bool success = false;
  ssize_t nbytes = pread(fd, buffer, size, offset);
  if (nbytes < 0)
    fprintf(stderr, "Failed to read from %s: %s\n", path, strerror(errno));
  else if (size_t(nbytes) != size)
    fprintf(stderr, "Short read from %s\n", path);
  else
    success = true;

  close(fd);
  return success;
}

static constexpr struct {
  const char *id;
  KoboModel model;
} kobo_model_ids[] = {
  { "SN-N514", KoboModel::AURA },
  { "SN-N236", KoboModel::AURA2 },
  { "SN-N705", KoboModel::MINI },
  { "SN-N905", KoboModel::TOUCH },
  { "SN-N587", KoboModel::TOUCH2 },
  { "SN-613A4", KoboModel::GLO },
  { "SN-N437", KoboModel::GLO_HD },
};

static KoboModel
DetectKoboModel(const char *p)
{
  for (const auto &i : kobo_model_ids)
    if (memcmp(p, i.id, strlen(i.id)) == 0)
      return i.model;

  return KoboModel::UNKNOWN;
}

KoboModel
DetectKoboModel()
{
  char buffer[16];
  if (!ReadFromFile("/dev/mmcblk0", 0x200, buffer, sizeof(buffer)))
    return KoboModel::UNKNOWN;

  return DetectKoboModel(buffer);
}
