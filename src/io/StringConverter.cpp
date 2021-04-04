/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "StringConverter.hpp"
#include "util/UTF8.hpp"

#include <cassert>
#include <cstring>
#include <stdexcept>

#ifdef _UNICODE
#include "system/Error.hxx"
#include <windows.h>
#endif

#ifdef _UNICODE

static void
iso_latin_1_to_tchar(TCHAR *dest, const char *src)
{
    do {
      *dest++ = (unsigned char)*src;
    } while (*src++ != '\0');
}

#endif

TCHAR *
StringConverter::Convert(char *narrow)
{
  assert(narrow != nullptr);

  // Check if there is byte order mark in front
  if (charset == Charset::AUTO || charset == Charset::UTF8) {
    char *p = SkipByteOrderMark(narrow);
    if (p != nullptr) {
      narrow = p;

      /* switch to UTF-8 now */
      charset = Charset::UTF8;
    }
  }

  if (charset == Charset::AUTO && !ValidateUTF8(narrow))
    /* invalid UTF-8 sequence detected: switch to ISO-Latin-1 */
    charset = Charset::ISO_LATIN_1;

#ifdef _UNICODE
  size_t narrow_length = strlen(narrow);

  TCHAR *t = tbuffer.get(narrow_length + 1);
  assert(t != nullptr);

  if (narrow_length == 0) {
    t[0] = _T('\0');
    return t;
  }

  switch (charset) {
  case Charset::ISO_LATIN_1:
    iso_latin_1_to_tchar(t, narrow);
    break;

  default:
    int length = MultiByteToWideChar(CP_UTF8, 0, narrow, narrow_length,
                                     t, narrow_length);
    if (length == 0)
      throw MakeLastError("Failed to convert string");

    t[length] = _T('\0');

    break;
  }

  return t;
#else
  switch (charset) {
    size_t buffer_size;
    const char *utf8;

  case Charset::ISO_LATIN_1:
    buffer_size = strlen(narrow) * 2 + 1;
    utf8 = Latin1ToUTF8(narrow, tbuffer.get(buffer_size), buffer_size);
    if (utf8 == nullptr)
      throw std::runtime_error("Latin-1 to UTF-8 conversion failed");

    return const_cast<char *>(utf8);

  case Charset::UTF8:
    if (!ValidateUTF8(narrow))
      /* abort on invalid UTF-8 sequence */
      throw std::runtime_error("Invalid UTF-8");

    /* fall through ... */
    gcc_fallthrough;

  case Charset::AUTO:
    return narrow;
  }

  /* unreachable */
  gcc_unreachable();
#endif
}
