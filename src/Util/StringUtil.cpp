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

#include "StringUtil.hpp"
#include "StringAPI.hxx"
#include "StringCompare.hxx"
#include "CharUtil.hpp"

#include <algorithm>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

char *
CopyString(char *gcc_restrict dest, const char *gcc_restrict src, size_t size)
{
  size_t length = strlen(src);
  if (length >= size)
    length = size - 1;

  char *p = std::copy_n(src, length, dest);
  *p = '\0';
  return p;
}

const char *
StripLeft(const char *p)
{
  while (IsWhitespaceNotNull(*p))
    ++p;

  return p;
}

const char *
StripRight(const char *p, const char *end)
{
  while (end > p && IsWhitespaceOrNull(end[-1]))
    --end;

  return end;
}

size_t
StripRight(const char *p, size_t length)
{
  while (length > 0 && IsWhitespaceOrNull(p[length - 1]))
    --length;

  return length;
}

void
StripRight(char *p)
{
  size_t old_length = strlen(p);
  size_t new_length = StripRight(p, old_length);
  p[new_length] = 0;
}

char *
NormalizeSearchString(char *gcc_restrict dest,
                      const char *gcc_restrict src)
{
  char *retval = dest;

  for (; !StringIsEmpty(src); ++src)
    if (IsAlphaNumericASCII(*src))
      *dest++ = ToUpperASCII(*src);

  *dest = '\0';

  return retval;
}
