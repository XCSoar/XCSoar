/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "ZipSource.hpp"

#include <zzip/util.h>

#ifdef _UNICODE
#include <windows.h>
#endif

ZipSource::ZipSource(struct zzip_dir *dir, const char *path)
{
  file = zzip_open_rb(dir, path);
}

ZipSource::ZipSource(const char *path)
{
  file = zzip_fopen(path, "rb");
}

#ifdef _UNICODE
ZipSource::ZipSource(const TCHAR *path)
  :file(NULL)
{
  char narrow_path[4096];

  int length = WideCharToMultiByte(CP_ACP, 0, path, -1,
                                   narrow_path, sizeof(narrow_path), NULL, NULL);
  if (length == 0)
    return;

  file = zzip_fopen(narrow_path, "rb");
}
#endif

ZipSource::~ZipSource()
{
  if (file != NULL)
    zzip_fclose(file);
}

long
ZipSource::size() const
{
  ZZIP_STAT st;
  return zzip_file_stat(file, &st) >= 0
    ? (long)st.st_size
    : -1l;
}

unsigned
ZipSource::read(char *p, unsigned n)
{
  zzip_ssize_t nbytes = zzip_read(file, p, n);
  return nbytes >= 0
    ? (unsigned)nbytes
    : 0;
}
