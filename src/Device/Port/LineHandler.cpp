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

#include "LineHandler.hpp"
#include "Util/CharUtil.hpp"

#include <string.h>

void
PortLineHandler::DataReceived(const void *_data, size_t length)
{
  assert(_data != NULL);
  assert(length > 0);

  const char *data = (const char *)_data, *end = data + length;

  do {
    /* append new data to buffer, as much as fits there */
    auto range = buffer.Write();
    if (range.second == 0) {
      /* overflow: reset buffer to recover quickly */
      buffer.Clear();
      continue;
    }

    size_t nbytes = std::min(size_t(range.second), size_t(end - data));
    memcpy(range.first, data, nbytes);
    data += nbytes;
    buffer.Append(nbytes);

    while (true) {
      /* read data from the buffer, to see if there's a newline
         character */
      range = buffer.Read();
      if (range.second == 0)
        break;

      char *newline = (char *)memchr(range.first, '\n', range.second);
      if (newline == NULL)
        /* no newline here: wait for more data */
        break;

      /* remove trailing whitespace, such as '\r' */
      char *end = newline;
      while (end > range.first && IsWhitespaceOrNull(end[-1]))
        --end;

      *end = '\0';
      LineReceived(range.first);

      buffer.Consume(newline - range.first + 1);
    }
  } while (data < end);
}
