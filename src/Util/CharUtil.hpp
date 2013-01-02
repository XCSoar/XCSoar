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

#ifndef XCSOAR_CHAR_UTIL_HPP
#define XCSOAR_CHAR_UTIL_HPP

#ifdef _UNICODE
#include <tchar.h>
#endif

constexpr
static inline bool
IsASCII(const char ch)
{
  return (unsigned char)ch < 0x80;
}

#ifdef _UNICODE
constexpr
static inline bool
IsASCII(const TCHAR ch)
{
  return (ch & ~0x7f) == 0;
}
#endif

static inline bool
IsWhitespaceOrNull(const char ch)
{
  return (unsigned char)ch <= 0x20;
}

static inline bool
IsWhitespaceNotNull(const char ch)
{
  return ch > 0 && ch <= 0x20;
}

#ifdef _UNICODE

static inline bool
IsWhitespaceOrNull(const TCHAR ch)
{
  return (unsigned)ch <= 0x20;
}

static inline bool
IsWhitespaceNotNull(const TCHAR ch)
{
  return ch > 0 && ch <= 0x20;
}

#endif /* _UNICODE */

constexpr
static inline bool
IsPrintableASCII(char ch)
{
  return (signed char)ch >= 0x20;
}

#ifdef _UNICODE
constexpr
static inline bool
IsPrintableASCII(TCHAR ch)
{
  return IsASCII(ch) && ch >= 0x20;
}
#endif

constexpr
static inline bool
IsDigitASCII(char ch)
{
  return ch >= '0' && ch <= '9';
}

constexpr
static inline bool
IsUpperAlphaASCII(char ch)
{
  return ch >= 'A' && ch <= 'Z';
}

constexpr
static inline bool
IsLowerAlphaASCII(char ch)
{
  return ch >= 'a' && ch <= 'z';
}

constexpr
static inline bool
IsAlphaASCII(char ch)
{
  return IsUpperAlphaASCII(ch) || IsLowerAlphaASCII(ch);
}

constexpr
static inline bool
IsAlphaNumericASCII(char ch)
{
  return IsAlphaASCII(ch) || IsDigitASCII(ch);
}

#ifdef _UNICODE

constexpr
static inline bool
IsDigitASCII(TCHAR ch)
{
  return ch >= _T('0') && ch <= _T('9');
}

constexpr
static inline bool
IsUpperAlphaASCII(TCHAR ch)
{
  return ch >= _T('A') && ch <= _T('Z');
}

constexpr
static inline bool
IsLowerAlphaASCII(TCHAR ch)
{
  return ch >= _T('a') && ch <= _T('z');
}

constexpr
static inline bool
IsAlphaASCII(TCHAR ch)
{
  return IsUpperAlphaASCII(ch) || IsLowerAlphaASCII(ch);
}

constexpr
static inline bool
IsAlphaNumericASCII(TCHAR ch)
{
  return IsAlphaASCII(ch) || IsDigitASCII(ch);
}

#endif

/**
 * Convert the specified ASCII character (0x00..0x7f) to upper case.
 * Unlike toupper(), it ignores the system locale.
 */
constexpr
static inline char
ToUpperASCII(char ch)
{
  return ch >= 'a' && ch <= 'z'
    ? (ch - ('a' - 'A'))
    : ch;
}

#endif
