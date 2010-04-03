/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
  const char *eol = (char *)memchr(data, '\n', length);
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
  Source<char>::Range range = source.read();
  if (range.second == 0)
    /* end of file */
    return NULL;

  std::pair<unsigned, unsigned> bounds =
    extract_line(range.first, range.second);
  source.consume(bounds.second);

  if (bounds.first >= range.second) {
    /* last line, not terminated by a line feed: copy to local buffer,
       because we want to append the null byte */
    char *line = last.get(range.second + 1);
    if (line == NULL)
      /* allocation has failed */
      return NULL;

    memcpy(line, range.first, range.second);
    line[range.second] = 0;
    return line;
  } else {
    /* there is space left for the null byte */
    range.first[bounds.first] = 0;
    return range.first;
  }
}

long
LineSplitter::size() const
{
  return source.size();
}

long
LineSplitter::tell() const
{
  return source.tell();
}
