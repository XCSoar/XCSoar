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

#include "Util.hpp"
#include "Util/CharUtil.hpp"

#include <algorithm>

#include <assert.h>
#include <string.h>

void
copy_padded(char *dest, size_t size, const char *src)
{
  assert(dest != nullptr);
  assert(size > 0);
  assert(src != nullptr);

  size_t src_length = strlen(src);
  if (src_length > size)
    src_length = size;

  memcpy(dest, src, src_length);
  memset(dest + src_length, ' ', size - src_length);
}

static char *
CopyUpper(char *dest, const char *src, const char *end)
{
  assert(dest != nullptr);
  assert(src != nullptr);
  assert(end >= src);

  while (src < end)
    *dest++ = ToUpperASCII(*src++);
  return dest;
}

void
CopyTerminatedUpper(char *dest, const char *src, size_t size)
{
  assert(dest != nullptr);
  assert(src != nullptr);
  assert(size > 0);

  const char *end = std::find(src, src + size, '\0');
  dest = CopyUpper(dest, src, end);
  *dest = '\0';
}

void
CopyPaddedUpper(char *dest, size_t size, const char *src)
{
  assert(dest != nullptr);
  assert(size > 0);
  assert(src != nullptr);

  size_t src_length = strlen(src);
  if (src_length > size)
    src_length = size;

  dest = CopyUpper(dest, src, src + src_length);
  memset(dest, ' ', size - src_length);
}
