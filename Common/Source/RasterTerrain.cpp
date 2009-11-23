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

#include "RasterTerrain.h"
#include "Math/FastMath.h"
#include "Registry.hpp"
#include "LocalPath.hpp"
#include "RasterMapRaw.hpp"
#include "RasterMapCache.hpp"
#include "RasterMapJPG2000.hpp"

#include "wcecompat/ts_string.h"

// General, open/close

void RasterTerrain::OpenTerrain(void)
{
  TCHAR  szFile[MAX_PATH] = TEXT("\0");

  GetRegistryString(szRegistryTerrainFile, szFile, MAX_PATH);

  TCHAR szOrigFile[MAX_PATH] = TEXT("\0");
  char zfilename[MAX_PATH];

  ExpandLocalPath(szFile);
  _tcscpy(szOrigFile, szFile);
  ContractLocalPath(szOrigFile);

  SetRegistryString(szRegistryTerrainFile, TEXT("\0"));
  unicode2ascii(szFile, zfilename, MAX_PATH);

  if (strlen(zfilename)==0) {
    static TCHAR  szMapFile[MAX_PATH] = TEXT("\0");
    GetRegistryString(szRegistryMapFile, szMapFile, MAX_PATH);
    ExpandLocalPath(szMapFile);
    _tcscpy(szFile,szMapFile);
    _tcscat(szFile,TEXT("/terrain.jp2"));
    unicode2ascii(szFile, zfilename, MAX_PATH);
  }

  // TODO code: Check locking, especially when reloading a file.
  // TODO bug: Fix cache method

  if (CreateTerrainMap(zfilename))
    SetRegistryString(szRegistryTerrainFile, szOrigFile);
}

bool
RasterTerrain::CreateTerrainMap(const char *zfilename)
{
  if (strstr(zfilename,".jp2")) {
    TerrainMap = RasterMapJPG2000::LoadFile(zfilename);
  } else {
    TerrainMap = RasterMapRaw::LoadFile(zfilename);
    if (TerrainMap == NULL)
      TerrainMap = RasterMapCache::LoadFile(zfilename);
  }

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

short RasterTerrain::GetTerrainHeight(const GEOPOINT &Location,
  const RasterRounding &rounding) {
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

bool RasterTerrain::IsPaged(void) const {
  if (TerrainMap) {
    return TerrainMap->IsPaged();
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

int RasterTerrain::GetEffectivePixelSize(double *pixel_D,
                                         const GEOPOINT &location) {
  if (TerrainMap) {
    return TerrainMap->GetEffectivePixelSize(pixel_D, location);
  } else {
    return 1;
  }
}

bool RasterTerrain::WaypointIsInTerrainRange(const GEOPOINT &location) {
  if (TerrainMap) {
    if ((location.Latitude<= TerrainMap->TerrainInfo.Top)&&
        (location.Latitude>= TerrainMap->TerrainInfo.Bottom)&&
        (location.Longitude<= TerrainMap->TerrainInfo.Right)&&
        (location.Longitude>= TerrainMap->TerrainInfo.Left)) {
      return true;
    } else {
      return false;
    }
  } else {
    return true;
  }
}

bool RasterTerrain::GetTerrainCenter(GEOPOINT *location) {
  if (TerrainMap) {
    location->Latitude = (TerrainMap->TerrainInfo.Top+
                          TerrainMap->TerrainInfo.Bottom)/2.0;
    location->Longitude = (TerrainMap->TerrainInfo.Left+
                           TerrainMap->TerrainInfo.Right)/2.0;
    return true;
  } else {
    return false;
  }
}
