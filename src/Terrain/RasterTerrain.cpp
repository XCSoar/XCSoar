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

#include "Terrain/RasterTerrain.hpp"
#include "Profile/Profile.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"

// General, open/close

RasterTerrain *
RasterTerrain::OpenTerrain(FileCache *cache)
{
  TCHAR szFile[MAX_PATH], world_file_buffer[MAX_PATH];
  const TCHAR *world_file;

  if (Profile::GetPath(szProfileTerrainFile, szFile)) {
    world_file = NULL;
  } else if (Profile::GetPath(szProfileMapFile, szFile)) {
    _tcscpy(world_file_buffer, szFile);
    _tcscat(world_file_buffer, _T(DIR_SEPARATOR_S "terrain.j2w"));
    world_file = world_file_buffer;

    _tcscat(szFile, _T("/terrain.jp2"));
  } else
    return NULL;

  RasterTerrain *rt = new RasterTerrain(szFile, world_file, cache);
  if (!rt->map.isMapLoaded()) {
    delete rt;
    return NULL;
  }

  return rt;
}
