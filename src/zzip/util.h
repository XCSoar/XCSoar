/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include <zzip/lib.h>

#include <fcntl.h>

/**
 * Opens a file inside a ZIP archive in read-only binary mode.
 *
 * @param dir optionally a ZZIP_DIR object
 * @param path the path inside the archive
 */
static inline ZZIP_FILE *
zzip_open_rb(ZZIP_DIR *dir, const char *path)
{
  int mode = O_RDONLY;
#ifdef O_NOCTTY
  mode |= O_NOCTTY;
#endif
#ifdef O_BINARY
  mode |= O_BINARY;
#endif

  return dir != NULL
    ? zzip_file_open(dir, path, mode)
    : zzip_open(path, mode);
}
