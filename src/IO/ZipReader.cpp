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

#include "ZipReader.hpp"

#include <zzip/util.h>

#include <stdexcept>

#include <stdio.h>

ZipReader::ZipReader(struct zzip_dir *dir, const char *path)
  :file(zzip_open_rb(dir, path))
{
  if (file == nullptr) {
    /* TODO: re-enable zziplib's error reporting, and improve this
       error message */
    char msg[256];
    snprintf(msg, sizeof(msg),
             "Failed to open '%s' from ZIP file", path);
    throw std::runtime_error(msg);
  }
}

ZipReader::~ZipReader()
{
  if (file != nullptr)
    zzip_file_close(file);
}

uint64_t
ZipReader::GetSize() const
{
  ZZIP_STAT st;
  return zzip_file_stat(file, &st) >= 0
    ? st.st_size
    : 0;
}

uint64_t
ZipReader::GetPosition() const
{
  return zzip_tell(file);
}

size_t
ZipReader::Read(void *data, size_t size)
{
  zzip_ssize_t nbytes = zzip_file_read(file, data, size);
  if (nbytes < 0)
    throw std::runtime_error("Failed to read from ZIP file");
  return nbytes;
}
