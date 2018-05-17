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

#ifndef XCSOAR_IGC_STRING_HPP
#define XCSOAR_IGC_STRING_HPP

#ifdef _UNICODE
#include <tchar.h>
#endif

/**
 * Is this a "reserved" character?
 *
 * Note that this doesn't check for CR/LF because these aren't allowed
 * within a line anyway.  No idea why these are mentioned in the
 * specification.
 *
 * @see IGC specification, section A6
 */
constexpr
static inline bool
IsReservedIGCChar(char ch)
{
  return ch == '$' || ch == '*' || ch == '!' || ch == '\\' ||
    ch == '^' || ch == '~';
}

/**
 * Is this a "valid" character?
 *
 * @see IGC specification, section A6
 */
constexpr
static inline bool
IsValidIGCChar(char ch)
{
  return ch >= 0x20 && ch <= 0x7e && !IsReservedIGCChar(ch);
}

#ifdef _UNICODE
constexpr
static inline bool
IsValidIGCChar(TCHAR ch)
{
  return ch >= 0x20 && ch <= 0x7e && !IsReservedIGCChar(char(ch));
}
#endif

/**
 * Copy a null-terminated string to a buffer to be written to an IGC
 * file.  If the string is too long for the buffer, it is truncated.
 * The destination buffer will not be null-terminated.
 */
char *
CopyIGCString(char *dest, char *dest_limit, const char *src);

#ifdef _UNICODE
char *
CopyIGCString(char *dest, char *dest_limit, const TCHAR *src);
#endif

#endif
