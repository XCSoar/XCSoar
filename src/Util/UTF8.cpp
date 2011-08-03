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

#include "Util/UTF8.hpp"

#include <algorithm>

static bool
IsContinuation(unsigned char ch)
{
  return (ch & 0xc0) == 0x80;
}

bool
ValidateUTF8(const char *p)
{
  for (; *p != 0; ++p) {
    unsigned char ch = *p;
    if (ch < 0x80)
      /* ASCII */
      continue;

    if (IsContinuation(ch))
      /* continuation without a prefix */
      return false;

    if ((ch & 0xe0) == 0xc0) {
      /* 1 continuation */
      if (!IsContinuation(*++p))
        return false;
    } else if ((ch & 0xf0) == 0xe0) {
      /* 2 continuations */
      if (!IsContinuation(*++p) || !IsContinuation(*++p))
        return false;
    } else if ((ch & 0xf8) == 0xf0) {
      /* 3 continuations */
      if (!IsContinuation(*++p) || !IsContinuation(*++p) ||
          !IsContinuation(*++p))
        return false;
    } else if ((ch & 0xfc) == 0xf8) {
      /* 4 continuations */
      if (!IsContinuation(*++p) || !IsContinuation(*++p) ||
          !IsContinuation(*++p) || !IsContinuation(*++p))
        return false;
    } else if ((ch & 0xfe) == 0xfc) {
      /* 5 continuations */
      if (!IsContinuation(*++p) || !IsContinuation(*++p) ||
          !IsContinuation(*++p) || !IsContinuation(*++p) ||
          !IsContinuation(*++p))
        return false;
    }
  }

  return true;
}

static bool
IsASCII(char ch)
{
  return (ch & 0x80) == 0;
}

static const char *
FindNonASCIIOrZero(const char *p)
{
  while (*p != 0 && IsASCII(*p))
    ++p;
  return p;
}

const char *
Latin1ToUTF8(const char *src, char *buffer, size_t buffer_size)
{
  const char *p = FindNonASCIIOrZero(src);
  if (*p == 0)
    /* everything is plain ASCII, we don't need to convert anything */
    return src;

  if ((size_t)(p - src) >= buffer_size)
    /* buffer too small */
    return NULL;

  const char *const end = buffer + buffer_size;
  char *q = std::copy(src, p, buffer);

  while (*p != 0) {
    unsigned char ch = *p++;

    if (IsASCII(ch)) {
      *q++ = ch;

      if (q >= end)
        /* buffer too small */
        return NULL;
    } else {
      if (q + 2 >= end)
        /* buffer too small */
        return NULL;

      *q++ = 0xc0 | (ch >> 6);
      *q++ = 0x80 | (ch & 0x3f);
    }
  }

  *q = 0;
  return buffer;
}
