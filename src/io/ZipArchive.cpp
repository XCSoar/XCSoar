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

#include "ZipArchive.hpp"
#include "system/ConvertPathName.hpp"
#include "util/RuntimeError.hxx"

#include <zzip/zzip.h>

ZipArchive::ZipArchive(Path path)
  :dir(zzip_dir_open(NarrowPathName(path), nullptr))
{
  if (dir == nullptr)
    throw FormatRuntimeError("Failed to open ZIP archive %s",
                             (const char *)NarrowPathName(path));
}

ZipArchive::~ZipArchive() noexcept
{
  if (dir != nullptr)
    zzip_dir_close(dir);
}

bool
ZipArchive::Exists(const char *name) const noexcept
{
  ZZIP_STAT st;
  return zzip_dir_stat(dir, name, &st, 0) == 0;
}

std::string
ZipArchive::NextName() noexcept
{
  ZZIP_DIRENT e;
  return zzip_dir_read(dir, &e)
    ? std::string(e.d_name)
    : std::string();
}
