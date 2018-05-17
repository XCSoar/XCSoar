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

#ifndef XCSOAR_STRING_UTIL_HPP
#define XCSOAR_STRING_UTIL_HPP

#include "Compiler.h"

#include <stddef.h>

#ifdef _UNICODE
#include "WStringUtil.hpp"
#endif

/**
 * Copy a string.  If the buffer is too small, then the string is
 * truncated.  This is a safer version of strncpy().
 *
 * @param size the size of the destination buffer (including the null
 * terminator)
 * @return a pointer to the null terminator
 */
gcc_nonnull_all
char *
CopyString(char *dest, const char *src, size_t size);

/**
 * Skips whitespace at the beginning of the string, and returns the
 * first non-whitespace character.  If the string has no
 * non-whitespace characters, then a pointer to the NULL terminator is
 * returned.
 */
gcc_pure gcc_nonnull_all
const char *
StripLeft(const char *p);

/**
 * Determine the string's end as if it was stripped on the right side.
 */
gcc_pure
const char *
StripRight(const char *p, const char *end);

/**
 * Determine the string's end as if it was stripped on the right side.
 */
gcc_pure
static inline char *
StripRight(char *p, char *end)
{
  return const_cast<char *>(StripRight((const char *)p,
                                       (const char *)end));
}

/**
 * Determine the string's length as if it was stripped on the right
 * side.
 */
gcc_pure
size_t
StripRight(const char *p, size_t length);

/**
 * Strips trailing whitespace.
 */
gcc_nonnull_all
void
StripRight(char *p);

/**
 * Normalize a string for searching.  This strips all characters
 * except letters and digits, folds case to a neutral form.  It is
 * possible to do this in-place (src==dest).
 *
 * @param dest the destination buffer; there must be enough room for
 * the source string and the trailing zero
 * @param src the source string
 * @return the destination buffer
 */
gcc_nonnull_all
char *
NormalizeSearchString(char *dest, const char *src);

#endif
