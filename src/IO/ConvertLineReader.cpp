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

#include "ConvertLineReader.hpp"
#include "Util/UTF8.hpp"

#include <string.h>

#ifdef _UNICODE
#include <windows.h>
#endif

ConvertLineReader::ConvertLineReader(std::unique_ptr<LineReader<char>> &&_source,
                                     Charset cs)
  :source(std::move(_source)),
   charset(cs)
{
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
ConvertLineReader::ReadLine()
{
  char *narrow = source->ReadLine();

  if (narrow == nullptr)
    return nullptr;

  // Check if there is byte order mark in front
  if (narrow[0] == (char)0xEF &&
      narrow[1] == (char)0xBB &&
      narrow[2] == (char)0xBF &&
      (charset == Charset::AUTO || charset == Charset::UTF8)) {
    // -> if so, skip it
    narrow += 3;

    /* if it was "AUTO", then explicitly switch to UTF-8 now */
    charset = Charset::UTF8;
  }

  if (charset == Charset::AUTO && !ValidateUTF8(narrow))
    /* invalid UTF-8 sequence detected: switch to ISO-Latin-1 */
    charset = Charset::ISO_LATIN_1;

#ifdef _UNICODE
  size_t narrow_length = strlen(narrow);

  TCHAR *t = tbuffer.get(narrow_length + 1);
  if (t == nullptr)
    return nullptr;

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
      return nullptr;

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
      return narrow;
    return const_cast<char *>(utf8);

  case Charset::UTF8:
    if (!ValidateUTF8(narrow))
      /* abort on invalid UTF-8 sequence */
      return nullptr;

    /* fall through ... */

  case Charset::AUTO:
    return narrow;
  }

  /* unreachable */
  gcc_unreachable();
#endif
}

long
ConvertLineReader::GetSize() const
{
  return source->GetSize();
}

long
ConvertLineReader::Tell() const
{
  return source->Tell();
}
