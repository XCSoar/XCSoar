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

#ifndef XCSOAR_VOLKSLOGGER_UTIL_HPP
#define XCSOAR_VOLKSLOGGER_UTIL_HPP

#include <cstddef>

/**
 * Copy a string to a fixed-size buffer.  The destination buffer is
 * not null-terminated, it is padded with spaces at the end if the
 * source string is shorter than the buffer.  If the source string is
 * longer than the destination buffer, it is clipped.
 */
void
copy_padded(char *dest, size_t size, const char *src);

/**
 * Copy a string from a fixed-size buffer that may not be
 * null-terminated, converting ASCII lower-case letters to upper case.
 * The destination buffer will be null-terminated.
 *
 * @param size the size of the source buffer
 */
void
CopyTerminatedUpper(char *dest, const char *src, size_t size);

/**
 * Copy a string to a fixed-size buffer, converting ASCII lower-case
 * letters to upper case.  The destination buffer is not
 * null-terminated, it is padded with spaces at the end if the source
 * string is shorter than the buffer.  If the source string is longer
 * than the destination buffer, it is clipped.
 */
void
CopyPaddedUpper(char *dest, size_t size, const char *src);

#endif
