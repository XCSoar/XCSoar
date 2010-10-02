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

#include "ConfiguredFile.hpp"
#include "FileLineReader.hpp"
#include "ZipLineReader.hpp"
#include "Profile/Profile.hpp"
#include "StringUtil.hpp"

#include <assert.h>

TLineReader *
OpenConfiguredTextFile(const TCHAR *profile_key, ConvertLineReader::charset cs)
{
  assert(profile_key != NULL);

  TCHAR path[MAX_PATH];
  if (!Profile::GetPath(profile_key, path))
    return NULL;

  FileLineReader *reader = new FileLineReader(path, cs);
  if (reader == NULL)
    return NULL;

  if (reader->error()) {
    delete reader;
    return NULL;
  }

  return reader;
}

static TLineReader *
OpenMapTextFile(const TCHAR *in_map_file, ConvertLineReader::charset cs)
{
  assert(in_map_file != NULL);

  TCHAR path[MAX_PATH];
  if (!Profile::GetPath(szProfileMapFile, path))
    return NULL;

  _tcscat(path, _T("/"));
  _tcscat(path, in_map_file);

  ZipLineReader *reader = new ZipLineReader(path, cs);
  if (reader == NULL)
    return NULL;

  if (reader->error()) {
    delete reader;
    return NULL;
  }

  return reader;
}

TLineReader *
OpenConfiguredTextFile(const TCHAR *profile_key, const TCHAR *in_map_file,
                       ConvertLineReader::charset cs)
{
  assert(profile_key != NULL);
  assert(in_map_file != NULL);

  TLineReader *reader = OpenConfiguredTextFile(profile_key, cs);
  if (reader == NULL)
    reader = OpenMapTextFile(in_map_file, cs);

  return reader;
}
