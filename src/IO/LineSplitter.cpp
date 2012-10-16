/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include <string.h>

static std::pair<unsigned, unsigned>
extract_line(const char *data, unsigned length)
{
  const char *eol = (const char *)memchr(data, '\n', length);
  if (eol == NULL)
    return std::pair<unsigned, unsigned>(length, length);

  length = eol + 1 - data;

  /* purge trailing carriage return characters */
  while (eol > data && eol[-1] == '\r')
    --eol;

  return std::pair<unsigned, unsigned>(eol - data, length);
}

char *
LineSplitter::read()
{
  /* is there enough data left in the buffer to read another line? */
  if (memchr(remaining.data, '\n', remaining.length) == NULL) {
    /* no: read more data from the Source */
    remaining = source.Read();
    if (remaining.IsEmpty())
      /* end of file */
      return NULL;
  }

  assert(!remaining.IsEmpty());

  Source<char>::Range range = remaining;
  std::pair<unsigned, unsigned> bounds =
    extract_line(range.data, range.length);
  source.Consume(bounds.second);
  remaining.data += bounds.second;
  remaining.length -= bounds.second;

  if (bounds.first >= range.length) {
    /* last line, not terminated by a line feed: copy to local buffer,
       because we want to append the null byte */
    char *line = last.get(range.length + 1);
    if (line == NULL)
      /* allocation has failed */
      return NULL;

    memcpy(line, range.data, range.length);
    line[range.length] = 0;
    return line;
  } else {
    /* there is space left for the null byte */
    range.data[bounds.first] = 0;
    return range.data;
  }
}

long
LineSplitter::size() const
{
  return source.GetSize();
}

long
LineSplitter::tell() const
{
  return source.Tell();
}
