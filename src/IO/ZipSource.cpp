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

#include "ZipSource.hpp"

#include <zzip/lib.h>

#ifdef _UNICODE
#include <windows.h>
#endif

ZipSource::ZipSource(const char *path)
  :BufferedSource<char>(4096)
{
  file = zzip_fopen(path, "rb");
}

#ifdef _UNICODE
ZipSource::ZipSource(const TCHAR *path)
  :BufferedSource<char>(4096), file(NULL)
{
  char narrow_path[4096];

  int length = WideCharToMultiByte(CP_UTF8, 0, path, -1,
                                   narrow_path, sizeof(narrow_path), NULL, NULL);
  if (length == 0)
    return;

  file = zzip_fopen(narrow_path, "rb");
}
#endif

ZipSource::~ZipSource()
{
  if (file != NULL)
    zzip_fclose(file);
}

long
ZipSource::size() const
{
  ZZIP_STAT st;
  return zzip_file_stat(file, &st) >= 0
    ? (long)st.st_size
    : -1l;
}

unsigned
ZipSource::read(char *p, unsigned n)
{
  zzip_ssize_t nbytes = zzip_read(file, p, n);
  return nbytes >= 0
    ? (unsigned)nbytes
    : 0;
}
