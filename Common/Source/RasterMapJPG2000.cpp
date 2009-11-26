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

#include "RasterMapJPG2000.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "UtilsSystem.hpp"

#ifdef __GNUC__
#define int_fast8_t jas_int_fast8_t
#endif

// JMW experimental
#include "jasper/jasper.h"
#include "jasper/jpc_rtc.h"
#include "wcecompat/ts_string.h"

void RasterMapJPG2000::SetFieldRounding(const double xr,
                                        const double yr,
                                        RasterRounding &rounding)
{
  RasterMap::SetFieldRounding(xr, yr, rounding);
  if (!isMapLoaded()) {
    return;
  }
  if ((rounding.Xrounding==1)&&(rounding.Yrounding==1)) {
    rounding.DirectFine = true;
    rounding.xlleft = (int)(TerrainInfo.Left*rounding.fXroundingFine)+128;
    rounding.xlltop  = (int)(TerrainInfo.Top*rounding.fYroundingFine)-128;
  } else {
    rounding.DirectFine = false;
  }
}

// Field access
short RasterMapJPG2000::_GetFieldAtXY(unsigned int lx,
                                      unsigned int ly) {

  return raster_tile_cache.GetField(lx,ly);
}

void RasterMapJPG2000::ServiceFullReload(const GEOPOINT &location) {
  ReloadJPG2000Full(location);
}

RasterMapJPG2000::RasterMapJPG2000() {
  TriggerJPGReload = false;
  jp2_filename[0] = '\0';
  DirectAccess = true;
  Paged = true;
  if (ref_count==0) {
    jas_init();
  }
  ref_count++;
}

RasterMapJPG2000::~RasterMapJPG2000() {
  ref_count--;
  if (ref_count==0) {
    jas_cleanup();
  }
}

int RasterMapJPG2000::ref_count = 0;

void RasterMapJPG2000::ReloadJPG2000Full(const GEOPOINT &location) {
  // load all 16 tiles...
  for (int i=0; i<MAX_ACTIVE_TILES; i++) {
    TriggerJPGReload = true;
    SetViewCenter(location);
  }
}

void RasterMapJPG2000::_ReloadJPG2000(void) {
  if (TriggerJPGReload) {
    TriggerJPGReload = false;

    raster_tile_cache.LoadJPG2000(jp2_filename);
    if (raster_tile_cache.GetInitialised()) {
      TerrainInfo.Left = raster_tile_cache.lon_min;
      TerrainInfo.Right = raster_tile_cache.lon_max;
      TerrainInfo.Top = raster_tile_cache.lat_max;
      TerrainInfo.Bottom = raster_tile_cache.lat_min;
      TerrainInfo.Columns = raster_tile_cache.GetWidth();
      TerrainInfo.Rows = raster_tile_cache.GetHeight();
      TerrainInfo.StepSize = (raster_tile_cache.lon_max -
                              raster_tile_cache.lon_min)
        /raster_tile_cache.GetWidth();
    }
  }
}

void RasterMapJPG2000::ReloadJPG2000(void) {
  Poco::ScopedRWLock protect(lock, true);
  _ReloadJPG2000();
}

void RasterMapJPG2000::SetViewCenter(const GEOPOINT &location)
{
  Poco::ScopedRWLock protect(lock, true);
  if (raster_tile_cache.GetInitialised()) {
    int x = lround((location.Longitude-TerrainInfo.Left)*TerrainInfo.Columns
                   /(TerrainInfo.Right-TerrainInfo.Left));
    int y = lround((TerrainInfo.Top-location.Latitude)*TerrainInfo.Rows
                   /(TerrainInfo.Top-TerrainInfo.Bottom));
    TriggerJPGReload |= raster_tile_cache.PollTiles(x, y);
  }
  if (TriggerJPGReload) {
    _ReloadJPG2000();
  }
}

bool
RasterMapJPG2000::Open(const char *zfilename)
{
  Poco::ScopedRWLock protect(lock, true);
  strcpy(jp2_filename,zfilename);

  // force first-time load
  TriggerJPGReload = true;
  _ReloadJPG2000();

  terrain_valid = raster_tile_cache.GetInitialised();
  if (!terrain_valid) {
    raster_tile_cache.Reset();
    max_field_value = 0;
  } else {
    max_field_value = raster_tile_cache.GetMaxElevation();
  }
  return terrain_valid;
}

RasterMapJPG2000 *
RasterMapJPG2000::LoadFile(const char *path)
{
  RasterMapJPG2000 *map = new RasterMapJPG2000();
  if (map == NULL)
    return NULL;

  if (!map->Open(path)) {
    delete map;
    return NULL;
  }

  return map;
}
