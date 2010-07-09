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
#include "Profile.hpp"
#include "LocalPath.hpp"
#include "RasterMapJPG2000.hpp"
#include "StringUtil.hpp"
#include "OS/PathName.hpp"

// General, open/close

void RasterTerrain::OpenTerrain(void)
{
  TCHAR szFile[MAX_PATH];

  if (Profile::Get(szProfileTerrainFile, szFile, MAX_PATH) &&
      !string_is_empty(szFile)) {
  } else if (Profile::Get(szProfileMapFile, szFile, MAX_PATH) &&
             !string_is_empty(szFile)) {
    _tcscat(szFile, _T("/terrain.jp2"));
  } else
    return;

  ExpandLocalPath(szFile);

  // TODO code: Check locking, especially when reloading a file.
  // TODO bug: Fix cache method

  CreateTerrainMap(NarrowPathName(szFile));
}

bool
RasterTerrain::CreateTerrainMap(const char *zfilename)
{
  TerrainMap = RasterMapJPG2000::LoadFile(zfilename);
  return TerrainMap != NULL;
}

void RasterTerrain::CloseTerrain(void)
{
  if (TerrainMap != NULL) {
    delete TerrainMap;
    TerrainMap = NULL;
  }
}

void RasterTerrain::Lock(void) {
  if (TerrainMap) {
    TerrainMap->LockRead();
  }
}

void RasterTerrain::Unlock(void) {
  if (TerrainMap) {
    TerrainMap->Unlock();
  }
}

short
RasterTerrain::GetTerrainHeight(const GEOPOINT &Location,
                                const RasterRounding &rounding) const
{
  if (TerrainMap) {
    return TerrainMap->GetField(Location, rounding);
  } else {
    return TERRAIN_INVALID;
  }
}

bool RasterTerrain::IsDirectAccess(void) const {
  if (TerrainMap) {
    return TerrainMap->IsDirectAccess();
  } else {
    return false;
  }
}

void RasterTerrain::ServiceCache(void) {
  if (TerrainMap) {
    TerrainMap->ServiceCache();
  }
}

void RasterTerrain::ServiceTerrainCenter(const GEOPOINT &location) {
  if (TerrainMap) {
    TerrainMap->SetViewCenter(location);
  }
}

void RasterTerrain::ServiceFullReload(const GEOPOINT &location) {
  if (TerrainMap) {
    TerrainMap->ServiceFullReload(location);
  }
}

int RasterTerrain::GetEffectivePixelSize(fixed &pixel_D,
                                         const GEOPOINT &location) const {
  if (TerrainMap) {
    return TerrainMap->GetEffectivePixelSize(pixel_D, location);
  } else {
    return 1;
  }
}

bool
RasterTerrain::WaypointIsInTerrainRange(const GEOPOINT &location) const
{
  if (TerrainMap) {
    if ((location.Latitude<= TerrainMap->TerrainInfo.TopLeft.Latitude)&&
        (location.Latitude>= TerrainMap->TerrainInfo.BottomRight.Latitude)&&
        (location.Longitude<= TerrainMap->TerrainInfo.BottomRight.Longitude)&&
        (location.Longitude>= TerrainMap->TerrainInfo.TopLeft.Longitude)) {
      return true;
    } else {
      return false;
    }
  } else {
    return true;
  }
}

bool
RasterTerrain::GetTerrainCenter(GEOPOINT *location) const
{
  if (TerrainMap) {
    *location = TerrainMap->TerrainInfo.TopLeft.interpolate(
      TerrainMap->TerrainInfo.BottomRight,
      fixed_half);
    return true;
  } else {
    return false;
  }
}
