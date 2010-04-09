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

#include "TextReader.hpp"

#include <string.h>

#ifdef _UNICODE
#include <windows.h>
#endif

TextReader::~TextReader() {}

long
TextReader::size() const
{
  return -1;
}

long
TextReader::tell() const
{
  return -1;
}

#ifdef _UNICODE

TCHAR *
TextReader::read()
{
  char *raw = read_raw_line();
  if (raw == NULL)
    return NULL;

  size_t raw_length = strlen(raw);

  TCHAR *t = tbuffer.get(raw_length + 1);
  if (t == NULL)
    return NULL;

  if (raw_length == 0) {
    t[0] = _T('\0');
    return t;
  }

  int length = MultiByteToWideChar(CP_UTF8, 0, raw, raw_length,
                                   t, raw_length);
  if (length == 0)
    return NULL;

  t[length] = _T('\0');
  return t;
}

#endif /* _UNICODE */

static long
FILE_size(FILE *file)
{
  if (fseek(file, 0L, SEEK_END) < 0)
    return -1;

  long size = ftell(file);
  fseek(file, 0L, SEEK_SET);

  return size;
}

FileTextReader::FileTextReader(const char *path)
{
  file = fopen(path, "rb");

  if (file != NULL)
    the_size = FILE_size(file);
}

#ifdef _UNICODE

FileTextReader::FileTextReader(const TCHAR *path)
{
  file = _tfopen(path, _T("rb"));

  if (file != NULL)
    the_size = FILE_size(file);
}

#endif

FileTextReader::~FileTextReader()
{
  if (file != NULL)
    fclose(file);
}

long
FileTextReader::size() const
{
  return the_size;
}

long
FileTextReader::tell() const
{
  return ftell(file);
}

static bool
is_line_terminator(char ch)
{
  return ch == '\n' || ch == '\r';
}

static bool
chomp(char *p)
{
  size_t length = strlen(p);
  if (length == 0)
    return false;

  if (!is_line_terminator(p[length - 1]))
    return false;

  --length;
  while (length > 0 && is_line_terminator(p[length - 1]))
    --length;

  p[length] = 0;
  return true;
}

char *
FileTextReader::read_raw_line()
{
  size_t max = 256;
  char *dest = buffer.get(max);
  if (dest == NULL || fgets(dest, max, file) == NULL)
    return NULL;

  if (chomp(dest))
    return dest;

  /* the line is longer than the specified buffer; try again with a
     larger buffer */
  size_t length = strlen(dest);
  max = 4096;
  dest = buffer.grow(length + 1, max);
  if (dest == NULL)
    return NULL;

  if (fgets(dest + length, max - length, file) == NULL || chomp(dest))
    return dest;

  /* still larger: last chance with an even larger buffer */
  length = strlen(dest);
  max = 16384;
  dest = buffer.grow(length + 1, max);
  if (dest == NULL)
    return NULL;

  fgets(dest + length, max - length, file);
  chomp(dest);
  return dest;
}
