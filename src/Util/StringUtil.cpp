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

#include "StringUtil.hpp"
#include "CharUtil.hpp"
#include "Compatibility/string.h"

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <algorithm>

const TCHAR *
string_after_prefix(const TCHAR *string, const TCHAR *prefix)
{
  assert(string != NULL);
  assert(prefix != NULL);

  size_t prefix_length = _tcslen(prefix);
  return _tcsncmp(string, prefix, prefix_length) == 0
    ? string + prefix_length
    : NULL;
}


const TCHAR *
string_after_prefix_ci(const TCHAR *string, const TCHAR *prefix)
{
  assert(string != NULL);
  assert(prefix != NULL);

  size_t prefix_length = _tcslen(prefix);
  return _tcsnicmp(string, prefix, prefix_length) == 0
    ? string + prefix_length
    : NULL;
}

TCHAR *
CopyString(TCHAR *dest, const TCHAR *src, size_t size)
{
  size_t length = _tcslen(src);
  if (length >= size)
    length = size - 1;

  TCHAR *p = std::copy(src, src + length, dest);
  *p = _T('\0');
  return p;
}

const TCHAR *
TrimLeft(const TCHAR *p)
{
  while (IsWhitespaceNotNull(*p))
    ++p;
  return p;
}

#ifdef _UNICODE
const char *
TrimLeft(const char *p)
{
  while (IsWhitespaceNotNull(*p))
    ++p;
  return p;
}
#endif

void
TrimRight(TCHAR *p)
{
  size_t length = _tcslen(p);

  while (length > 0 && IsWhitespaceOrNull(p[length - 1]))
    --length;

  p[length] = 0;
}

#ifdef _UNICODE
void
TrimRight(char *p)
{
  size_t length = strlen(p);

  while (length > 0 && IsWhitespaceOrNull(p[length - 1]))
    --length;

  p[length] = 0;
}
#endif

TCHAR *
normalize_search_string(TCHAR *dest, const TCHAR *src)
{
  TCHAR *retval = dest;

  for (; !string_is_empty(src); ++src)
    if (_istalnum(*src))
      *dest++ = _totupper(*src);

  *dest = _T('\0');

  return retval;
}
