/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#include "XCSoar.h"
#include "RasterWeather.h"
#include "LogFile.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Math/FastMath.h"
#include "Registry.hpp"
#include "LocalPath.hpp"

#include "wcecompat/ts_string.h"

///////////////////// General, open/close ///////////////////////////////

void RasterTerrain::OpenTerrain(void)
{
  terrain_initialised = false;

  StartupStore(TEXT("OpenTerrain\n"));
  CreateProgressDialog(gettext(TEXT("Loading Terrain File...")));
  SetProgressStepSize(2);

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

  if (CreateTerrainMap(zfilename)) {
    SetRegistryString(szRegistryTerrainFile, szOrigFile);
    terrain_initialised = true;
  } else {
    if (TerrainMap) {
      TerrainMap->Close();
      delete TerrainMap;
      TerrainMap = NULL;
    }
    terrain_initialised = false;
  }
}


bool RasterTerrain::CreateTerrainMap(char *zfilename) {
  if (strstr(zfilename,".jp2")) {
    TerrainMap = new RasterMapJPG2000();
    if (!TerrainMap)
      return false;
    return TerrainMap->Open(zfilename);
  }
  TerrainMap = new RasterMapRaw();
  if (!TerrainMap) {
    return false;
  }
  if (TerrainMap->Open(zfilename)) {
    return true;
  }
  TerrainMap->Close();
  delete TerrainMap;

  TerrainMap = new RasterMapCache();
  if (!TerrainMap) {
    return false;
  }
  if (TerrainMap->Open(zfilename)) {
    return true;
  }
  return false;
}


void RasterTerrain::CloseTerrain(void)
{
  StartupStore(TEXT("CloseTerrain\n"));
  // TODO code: lock it first?

  if (terrain_initialised) {

    if (TerrainMap) {
      TerrainMap->Close();
      delete TerrainMap;
      TerrainMap = NULL;
    }
    terrain_initialised = false;
  }
}


////////

void RasterTerrain::Lock(void) {
  if (TerrainMap) {
    TerrainMap->Lock();
  }
}

void RasterTerrain::Unlock(void) {
  if (TerrainMap) {
    TerrainMap->Unlock();
  }
}

short RasterTerrain::GetTerrainHeight(const double &Latitude,
                                      const double &Longitude) {
  if (TerrainMap) {
    return TerrainMap->GetField(Latitude, Longitude);
  } else {
    return TERRAIN_INVALID;
  }
}

bool RasterTerrain::IsDirectAccess(void) {
  if (TerrainMap) {
    return TerrainMap->IsDirectAccess();
  } else {
    return false;
  }
}


bool RasterTerrain::IsPaged(void) {
  if (TerrainMap) {
    return TerrainMap->IsPaged();
  } else {
    return false;
  }
}


void RasterTerrain::ServiceCache(void) {
  Lock();
  if (TerrainMap) {
    TerrainMap->ServiceCache();
  }
  Unlock();
}

void RasterTerrain::SetTerrainRounding(double x, double y) {
  if (TerrainMap) {
    TerrainMap->SetFieldRounding(x, y);
  }
}

void RasterTerrain::ServiceTerrainCenter(double lat, double lon) {
  Lock();

  if (TerrainMap) {
    TerrainMap->SetViewCenter(lat, lon);
  }
  Unlock();
}


void RasterTerrain::ServiceFullReload(double lat, double lon) {

  Lock();
  if (TerrainMap) {
    CreateProgressDialog(gettext(TEXT("Loading terrain tiles...")));
    TerrainMap->ServiceFullReload(lat, lon);
  }
  Unlock();
}


int RasterTerrain::GetEffectivePixelSize(double *pixel_D,
                                         double latitude, double longitude) {
  if (TerrainMap) {
    return TerrainMap->GetEffectivePixelSize(pixel_D, latitude, longitude);
  } else {
    return 1;
  }
}


bool RasterTerrain::WaypointIsInTerrainRange(double latitude,
                                             double longitude) {
  if (TerrainMap) {
    if ((latitude<= TerrainMap->TerrainInfo.Top)&&
        (latitude>= TerrainMap->TerrainInfo.Bottom)&&
        (longitude<= TerrainMap->TerrainInfo.Right)&&
        (longitude>= TerrainMap->TerrainInfo.Left)) {
      return true;
    } else {
      return false;
    }
  } else {
    return true;
  }
}


bool RasterTerrain::GetTerrainCenter(double *latitude,
                                     double *longitude) {
  if (TerrainMap) {
    *latitude = (TerrainMap->TerrainInfo.Top+
                 TerrainMap->TerrainInfo.Bottom)/2.0;
    *longitude = (TerrainMap->TerrainInfo.Left+
                  TerrainMap->TerrainInfo.Right)/2.0;
    return true;
  } else {
    return false;
  }
}


