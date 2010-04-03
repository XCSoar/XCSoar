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

#include "ZipTextReader.hpp"

#include <assert.h>
#include <string.h>

#ifdef _UNICODE
#include <windows.h>
#endif

ZipTextReader::ZipTextReader(const char *path)
  :buffer(4096)
{
  file = zzip_fopen(path, "rb");
}

#ifdef _UNICODE
ZipTextReader::ZipTextReader(const TCHAR *path)
  :file(NULL), buffer(4096)
{
  FifoBuffer<char>::Range range = buffer.write();
  assert(range.second > 0);

  int length = WideCharToMultiByte(CP_UTF8, 0, path, -1,
                                   range.first, range.second, NULL, NULL);
  if (length == 0)
    return;

  file = zzip_fopen(range.first, "rb");
}
#endif

ZipTextReader::~ZipTextReader()
{
  if (file != NULL)
    zzip_fclose(file);
}

static bool
is_line_terminator(char ch)
{
  return ch == '\n' || ch == '\r';
}

char *
ZipTextReader::extract_line()
{
  FifoBuffer<char>::Range range = buffer.read();
  char *newline = (char *)memchr(range.first, '\n', range.second);
  if (newline == NULL)
    return NULL;

  buffer.consume(newline + 1 - range.first);

  while (newline > range.first && is_line_terminator(newline[-1]))
    --newline;
  *newline = 0;

  return range.first;
}

char *
ZipTextReader::extract_rest()
{
  FifoBuffer<char>::Range range = buffer.read();
  if (range.second == 0)
    return NULL;

  /* null-terminate the buffer */

  range = buffer.write();
  assert(range.second > 0);

  range.first[0] = '\0';

  /* return the whole buffer */

  range = buffer.read();
  assert(range.second > 0);

  buffer.consume(range.second);
  return range.first;
}

bool
ZipTextReader::fill_buffer()
{
  FifoBuffer<char>::Range range = buffer.write();
  if (range.second <= 1)
    return false;

  zzip_ssize_t nbytes = zzip_read(file, range.first, range.second);
  if (nbytes < 0)
    return false;

  buffer.append(nbytes);
  return true;
}

char *
ZipTextReader::read_raw_line()
{
  char *line = extract_line();
  if (line != NULL)
    return line;

  if (!fill_buffer())
    return NULL;

  line = extract_line();
  if (line == NULL)
    line = extract_rest();

  return line;
}

long
ZipTextReader::size() const
{
  return zzip_file_size(file);
}

long
ZipTextReader::tell() const
{
  return zzip_tell(file);
}
