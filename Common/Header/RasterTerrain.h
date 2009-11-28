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

#ifndef RASTERTERRAIN_H
#define RASTERTERRAIN_H

#include "Sizes.h"
#include "GeoPoint.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class RasterMap;
class RasterRounding;

class RasterTerrain {
public:

  RasterTerrain():
    TerrainMap(NULL) {
  };

   void SetViewCenter(const double &Latitude,
                      const double &Longitude);
   void OpenTerrain();
   void CloseTerrain();

  bool isTerrainLoaded() const {
    return TerrainMap != NULL;
  }
   RasterMap* TerrainMap;
   bool CreateTerrainMap(const char *path);

 public:
   void Lock(void); // should be protected, friend of TerrainDataClient
   void Unlock(void); // should be protected, friend of TerrainDataClient

   const RasterMap* GetMap() const {
     return TerrainMap;
   }
   short GetTerrainHeight(const GEOPOINT &location,
                          const RasterRounding &rounding) const;
   bool IsDirectAccess(void) const;
   bool IsPaged(void) const;
   void ServiceCache();
   void ServiceTerrainCenter(const GEOPOINT &location);
   void ServiceFullReload(const GEOPOINT &location);
   int GetEffectivePixelSize(double *pixel_D, const GEOPOINT &location) const;
   bool WaypointIsInTerrainRange(const GEOPOINT &location) const;
   bool GetTerrainCenter(GEOPOINT *location) const;
};

#endif
