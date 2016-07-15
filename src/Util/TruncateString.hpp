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

#ifndef XCSOAR_TRUNCATE_STRING_HPP
#define XCSOAR_TRUNCATE_STRING_HPP

#include <tchar.h>
#include <stddef.h>

/**
 * Copy a string to a buffer, truncating it if the buffer is not large
 * enough.  No partial (UTF-8) multi-byte sequence will be copied.
 *
 * @param dest_size the total size of the destination buffer, which
 * includes the null byte
 * @return a pointer to the end of the destination string
 */
char *
CopyTruncateString(char *dest, size_t dest_size, const char *src);

#ifdef _UNICODE
TCHAR *
CopyTruncateString(TCHAR *dest, size_t dest_size, const TCHAR *src);
#endif

/**
 * Copy a string to a buffer, truncating it if the buffer is not large
 * enough.  At most #truncate characters will be copied.  No partial
 * (UTF-8) multi-byte sequence will be copied.
 *
 * @param dest_size the total size of the destination buffer, which
 * includes the null byte
 * @param truncate the maximum number of characters (not bytes) to
 * copy
 * @return a pointer to the end of the destination string
 */
TCHAR *
CopyTruncateString(TCHAR *dest, size_t dest_size,
                   const TCHAR *src, size_t truncate);

#endif
