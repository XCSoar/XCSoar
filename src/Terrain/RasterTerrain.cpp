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

#include "Terrain/RasterTerrain.hpp"
#include "Profile/Profile.hpp"
#include "OS/PathName.hpp"
#include "Compatibility/path.h"

#include <windef.h> /* for MAX_PATH */

// General, open/close

RasterTerrain *
RasterTerrain::OpenTerrain(FileCache *cache, OperationEnvironment &operation)
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

  RasterTerrain *rt = new RasterTerrain(szFile, world_file, cache, operation);
  if (!rt->map.isMapLoaded()) {
    delete rt;
    return NULL;
  }

  return rt;
}
