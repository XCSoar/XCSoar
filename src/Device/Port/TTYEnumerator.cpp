/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "TTYEnumerator.hpp"
#include "util/CharUtil.hxx"
#include "util/StringCompare.hxx"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <tchar.h>
#include <libgen.h>

static bool
IsDirectory(const char *path) noexcept
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool
IsSymlink(const char *path) noexcept
{
    struct stat st;
    /* use lstat() to get stat() of link not of linked target */
    return lstat(path, &st) == 0 && S_ISLNK(st.st_mode);
}

[[gnu::pure]]
static bool
CheckTTYName(const char *name) noexcept
{
  /*
   * rfcomm devices do not have a 'device/driver' file that is used
   * to exclude virtual/pseudo terminals
   */
  if (StringStartsWith(name, "rfcomm"))
    return true;

  /*
   * we assume only entries with existing 'device/driver' folder
   * aren't virtual/pseudo terminals, as they are associated with a
   * kernel driver
   */
  const char *driver_format = "/sys/class/tty/%s/device/driver";
  char driver[128];
  if (snprintf(driver, sizeof(driver), driver_format, name) >= (int)sizeof(driver))
	  /* ignore truncated file path */
    return false;

  if (IsDirectory(driver))
    return true;

  return false;
}

[[gnu::pure]]
static bool
CheckTTYDevice(const char *device, struct dirent *ent) noexcept
{
  if(!IsSymlink(device))
    return CheckTTYName(ent->d_name);

  char* real_path = realpath(device, nullptr);
  if (real_path == nullptr)
    return false;

  bool result = CheckTTYName(basename(real_path));
  free(real_path);
  return result;
}

const char *
TTYEnumerator::Next() noexcept
{
  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    if (snprintf(path, sizeof(path), "/dev/%s", ent->d_name) >= (int)sizeof(path))
      /* truncated - ignore */
      continue;

    if (!CheckTTYDevice(path, ent))
      continue;

    if (access(path, R_OK|W_OK) == 0 && access(path, X_OK) < 0)
      return path;
  }

  return nullptr;
}
