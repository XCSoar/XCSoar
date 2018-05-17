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

#include "TruncateString.hpp"
#include "StringAPI.hxx"
#include "UTF8.hpp"

#include <algorithm>

#include <assert.h>

char *
CopyTruncateString(char *dest, size_t dest_size, const char *src)
{
  assert(dest != nullptr);
  assert(dest_size > 0);
  assert(src != nullptr);

  size_t src_length = StringLength(src);
  size_t copy = std::min(src_length, dest_size - 1);

  auto *p = std::copy_n(src, copy, dest);
  *p = _T('\0');
  return CropIncompleteUTF8(dest);
}

#ifdef _UNICODE

TCHAR *
CopyTruncateString(TCHAR *dest, size_t dest_size, const TCHAR *src)
{
  assert(dest != nullptr);
  assert(dest_size > 0);
  assert(src != nullptr);

  size_t src_length = StringLength(src);
  size_t copy = std::min(src_length, dest_size - 1);

  auto *p = std::copy_n(src, copy, dest);
  *p = _T('\0');
  return p;
}

#endif

TCHAR *
CopyTruncateString(TCHAR *dest, size_t dest_size,
                   const TCHAR *src, size_t truncate)
{
  assert(dest != nullptr);
  assert(dest_size > 0);
  assert(src != nullptr);

#ifdef _UNICODE
  size_t src_length = StringLength(src);
  size_t copy = std::min({src_length, truncate, dest_size - 1});

  auto *p = std::copy_n(src, copy, dest);
  *p = _T('\0');
  return p;
#else
  return CopyTruncateStringUTF8(dest, dest_size, src, truncate);
#endif
}
