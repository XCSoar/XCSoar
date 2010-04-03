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

#include "ConvertLineReader.hpp"

#ifdef _UNICODE
#include <windows.h>
#endif

ConvertLineReader::ConvertLineReader(LineReader<char> &_source, charset cs)
  :source(_source)
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

TCHAR *
ConvertLineReader::read()
{
  char *narrow = source.read();

#ifdef _UNICODE
  if (narrow == NULL)
    return NULL;

  size_t narrow_length = strlen(narrow);

  TCHAR *t = tbuffer.get(narrow_length + 1);
  if (t == NULL)
    return NULL;

  if (narrow_length == 0) {
    t[0] = _T('\0');
    return t;
  }

  int length = MultiByteToWideChar(CP_UTF8, 0, narrow, narrow_length,
                                   t, narrow_length);
  if (length == 0)
    return NULL;

  t[length] = _T('\0');
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
