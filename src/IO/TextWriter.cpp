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

#include "TextWriter.hpp"

#include <assert.h>
#include <string.h>

#ifdef _UNICODE
#include <windows.h>
#endif

TextWriter::TextWriter(const char *path, bool append)
  :file(path, append ? "ab" : "wb") {}

#ifdef _UNICODE

TextWriter::TextWriter(const TCHAR *path, bool append)
  :file(path, append ? _T("ab") : _T("wb")) {}

#endif

#ifdef _UNICODE

bool
TextWriter::write(const TCHAR *s, size_t src_length)
{
  if (src_length == 0)
    /* empty string, nothing to do */
    return true;

  size_t dest_size = src_length * 5;
  char *dest = convert_buffer.get(dest_size);
  if (dest == NULL)
    return false;

  int length = WideCharToMultiByte(CP_UTF8, 0, s, src_length,
                                   dest, dest_size, NULL, NULL);
  if (length == 0)
    return false;

  return write(dest, length);
}

bool
TextWriter::write(const TCHAR *s)
{
  assert(_tcschr(s, _T('\r')) == NULL);
  assert(_tcschr(s, _T('\n')) == NULL);

  return write(s, _tcslen(s));
}

bool
TextWriter::vprintf(const TCHAR *fmt, va_list ap)
{
  assert(_tcschr(fmt, _T('\r')) == NULL);
  assert(_tcschr(fmt, _T('\n')) == NULL);

  /* assume 4 kB is enough for one line */
  size_t buffer_size = 4096;
  TCHAR *buffer = format_buffer.get(buffer_size);
  if (buffer == NULL)
    return false;

  _vsntprintf(buffer, buffer_size, fmt, ap);
  return write(buffer);
}

#endif /* _UNICODE */

void
TextWriter::printf(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

#ifdef _UNICODE
void
TextWriter::printf(const TCHAR *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}
#endif

void
TextWriter::printfln(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintfln(fmt, ap);
  va_end(ap);
}

#ifdef _UNICODE
void
TextWriter::printfln(const TCHAR *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintfln(fmt, ap);
  va_end(ap);
}
#endif
