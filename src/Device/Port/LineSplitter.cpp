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

#include "LineSplitter.hpp"
#include "Util/CharUtil.hpp"

#include <algorithm>

#include <string.h>

constexpr
static bool
IsInsaneChar(char ch)
{
  return (unsigned char)ch < 0x20;
}

/**
 * Replace all control characters with a regular space character.
 */
static void
SanitiseLine(char *const begin, char *const end)
{
  std::replace_if(begin, end, IsInsaneChar, ' ');
}

void
PortLineSplitter::DataReceived(const void *_data, size_t length)
{
  assert(_data != NULL);
  assert(length > 0);

  const char *data = (const char *)_data, *end = data + length;

  do {
    /* append new data to buffer, as much as fits there */
    auto range = buffer.Write();
    if (range.IsEmpty()) {
      /* overflow: reset buffer to recover quickly */
      buffer.Clear();
      continue;
    }

    size_t nbytes = std::min(size_t(range.size), size_t(end - data));
    memcpy(range.data, data, nbytes);
    data += nbytes;
    buffer.Append(nbytes);

    while (true) {
      /* read data from the buffer, to see if there's a newline
         character */
      range = buffer.Read();
      if (range.IsEmpty())
        break;

      char *newline = (char *)memchr(range.data, '\n', range.size);
      if (newline == NULL)
        /* no newline here: wait for more data */
        break;

      /* remove trailing whitespace, such as '\r' */
      char *end = newline;
      while (end > range.data && IsWhitespaceOrNull(end[-1]))
        --end;

      *end = '\0';

      SanitiseLine(range.data, end);

      const char *line = range.data;

      /* if there are NUL bytes in the line, skip to after the last
         one, to avoid conflicts with NUL terminated C strings due to
         binary garbage */
      const void *nul;
      while ((nul = memchr(line, 0, end - line)) != NULL)
        line = (const char *)nul + 1;

      LineReceived(line);

      buffer.Consume(newline - range.data + 1);
    }
  } while (data < end);
}
