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
#include "Util/UTF8.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

ConvertLineReader::ConvertLineReader(LineReader<char> &_source, charset cs)
  :source(_source),
   m_charset(cs)
{
#ifdef _UNICODE
  switch (cs) {
  case UTF8:
    code_page = CP_UTF8;
    break;

  default:
    code_page = CP_UTF8;
  }
#endif
}

#ifdef _UNICODE

static void
iso_latin_1_to_tchar(TCHAR *dest, const char *src)
{
    do {
      *dest++ = *src;
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
      narrow[2] == (char)0xBF &&
      (m_charset == AUTO || m_charset == UTF8)) {
    // -> if so, skip it
    narrow += 3;

    /* if it was "AUTO", then explicitly switch to UTF-8 now */
    m_charset = UTF8;
  }

  if (m_charset == AUTO && !ValidateUTF8(narrow))
    /* invalid UTF-8 sequence detected: switch to ISO-Latin-1 */
    m_charset = ISO_LATIN_1;

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
  switch (m_charset) {
    size_t buffer_size;
    const char *utf8;

  case ISO_LATIN_1:
    buffer_size = strlen(narrow) * 2 + 1;
    utf8 = Latin1ToUTF8(narrow, tbuffer.get(buffer_size), buffer_size);
    if (utf8 == NULL)
      return narrow;
    return const_cast<char *>(utf8);

  case AUTO:
  case UTF8:
    return narrow;
  }

  /* unreachable */
  assert(false);
  return NULL;
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
