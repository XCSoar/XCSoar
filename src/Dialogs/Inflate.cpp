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

#include "Inflate.hpp"

#include <zlib.h>

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

/**
 * Uncompress the given buffer, and return it as a C string.  The
 * caller is responsible for freeing it with delete[].
 */
char *
InflateToString(const void *compressed, size_t length)
{
  size_t buffer_size = length * 8;
  char *buffer = new char[buffer_size];

  z_stream strm;
  memset(&strm, 0, sizeof(strm));
  strm.zalloc = 0;
  strm.zfree = 0;
  /* we need the const_cast because Android NDK r10e contains ZLib
     1.2.3 headers without ZLIB_CONST support */
  strm.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(compressed));
  strm.avail_in = length;
  strm.next_out = reinterpret_cast<Bytef *>(buffer);
  strm.avail_out = buffer_size - 1;

  int result = inflateInit2(&strm, 16+MAX_WBITS);
  if (result != Z_OK) {
    delete[] buffer;
    return NULL;
  }

  result = inflate(&strm, Z_NO_FLUSH);
  inflateEnd(&strm);
  if (result != Z_STREAM_END) {
    delete[] buffer;
    return NULL;
  }

  assert((size_t)strm.avail_out < buffer_size);

  buffer[buffer_size - 1 - strm.avail_out] = 0;
  return buffer;
}
