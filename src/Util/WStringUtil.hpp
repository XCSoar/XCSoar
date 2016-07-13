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

#ifndef WSTRING_UTIL_HPP
#define WSTRING_UTIL_HPP

#include "Compiler.h"

#include <wchar.h>

/**
 * Copy a string.  If the buffer is too small, then the string is
 * truncated.  This is a safer version of strncpy().
 *
 * @param size the size of the destination buffer (including the null
 * terminator)
 * @return a pointer to the null terminator
 */
gcc_nonnull_all
wchar_t *
CopyString(wchar_t *dest, const wchar_t *src, size_t size);

gcc_pure gcc_nonnull_all
const wchar_t *
StripLeft(const wchar_t *p);

gcc_pure
const wchar_t *
StripRight(const wchar_t *p, const wchar_t *end);

/**
 * Determine the string's end as if it was stripped on the right side.
 */
gcc_pure
static inline wchar_t *
StripRight(wchar_t *p, wchar_t *end)
{
  return const_cast<wchar_t *>(StripRight((const wchar_t *)p,
                                        (const wchar_t *)end));
}

/**
 * Determine the string's length as if it was stripped on the right
 * side.
 */
gcc_pure
size_t
StripRight(const wchar_t *p, size_t length);

gcc_nonnull_all
void
StripRight(wchar_t *p);

gcc_nonnull_all
wchar_t *
NormalizeSearchString(wchar_t *dest, const wchar_t *src);

#endif
