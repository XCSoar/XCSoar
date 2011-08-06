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

#include "ConvertLineReader.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

ConvertLineReader::ConvertLineReader(LineReader<char> &_source, charset cs)
  :source(_source)
#ifdef _UNICODE
  , m_charset(cs)
#endif
{
#ifdef _UNICODE
  switch (cs) {
  case UTF8:
    code_page = CP_UTF8;
    break;

  case WINDOWS_1252:
    code_page = CP_ACP;
    break;

  default:
    code_page = CP_UTF8;
  }
#else
  // XXX initialize iconv?
#endif
}

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
ConvertLineReader::read()
{
  char *narrow = source.read();

  if (narrow == NULL)
    return NULL;

  // Check if there is byte order mark in front
  if (narrow[0] == (char)0xEF &&
      narrow[1] == (char)0xBB &&
      narrow[2] == (char)0xBF)
    // -> if so, skip it
    narrow += 3;

#ifdef _UNICODE
  size_t narrow_length = strlen(narrow);

  TCHAR *t = tbuffer.get(narrow_length + 1);
  if (t == NULL)
    return NULL;

  if (narrow_length == 0) {
    t[0] = _T('\0');
    return t;
  }

  switch (m_charset) {
  case ISO_LATIN_1:
    iso_latin_1_to_tchar(t, narrow);
    break;

  default:
    int length = MultiByteToWideChar(code_page, 0, narrow, narrow_length,
                                     t, narrow_length);
    if (length == 0)
      return NULL;

    t[length] = _T('\0');

    break;
  }

  return t;
#else
  // XXX call iconv?
  return narrow;
#endif
}

long
ConvertLineReader::size() const
{
  return source.size();
}

long
ConvertLineReader::tell() const
{
  return source.tell();
}
