/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_UTIL_UTF8_HPP
#define XCSOAR_UTIL_UTF8_HPP

#include "Compiler.h"

#include <stddef.h>

/**
 * Is this a valid UTF-8 string?
 */
gcc_pure  gcc_nonnull_all
bool
ValidateUTF8(const char *p);

/**
 * Convert the specified string from ISO-8859-1 to UTF-8.
 *
 * @return the UTF-8 version of the source string; may return #src if
 * there are no non-ASCII characters; returns nullptr if the destination
 * buffer is too small
 */
gcc_pure  gcc_nonnull_all
const char *
Latin1ToUTF8(const char *src, char *buffer, size_t buffer_size);

/**
 * Convert the specified character from ISO-8859-1 to UTF-8 and write
 * it to the buffer. buffer must have a length of at least 2!
 *
 * @return A pointer to the buffer plus the added character(s)
 */
gcc_nonnull_all
char *
Latin1ToUTF8(unsigned char ch, char *buffer);

/**
 * Returns the number of characters in the string.  This is different
 * from strlen(), which counts the number of bytes.
 */
gcc_pure gcc_nonnull_all
size_t
LengthUTF8(const char *p);

#endif
