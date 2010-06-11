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

#ifdef __GNUC__
#define int_fast8_t jas_int_fast8_t
#endif

#include "jasper/jasper.h"

void
RasterMapJPG2000::SetFieldRounding(const GEOPOINT& delta,
                                   RasterRounding &rounding) const
{
  RasterMap::SetFieldRounding(delta, rounding);
  if (!isMapLoaded()) {
    return;
  }
  if ((rounding.Xrounding==1)&&(rounding.Yrounding==1)) {
    rounding.DirectFine = true;
    rounding.xlleft = (int)(TerrainInfo.TopLeft.Longitude.value_native() * rounding.fXroundingFine) + 128;
    rounding.xlltop  = (int)(TerrainInfo.TopLeft.Latitude.value_native() * rounding.fYroundingFine) - 128;
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
  TriggerJPGReload = true;
  FullJPGReload= true;
  _SetViewCenter(location);
  FullJPGReload= false;
}

RasterMapJPG2000::RasterMapJPG2000() {
  TriggerJPGReload = false;
  jp2_filename[0] = '\0';
  DirectAccess = true;
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

void RasterMapJPG2000::_ReloadJPG2000() {
  TriggerJPGReload = false;

  raster_tile_cache.LoadJPG2000(jp2_filename, FullJPGReload);
  FullJPGReload = false;

  if (!raster_tile_cache.GetInitialised())
    return;

  TerrainInfo.TopLeft.Longitude =
    Angle::degrees((fixed)raster_tile_cache.lon_min);
  TerrainInfo.BottomRight.Longitude =
    Angle::degrees((fixed)raster_tile_cache.lon_max);
  TerrainInfo.TopLeft.Latitude =
    Angle::degrees((fixed)raster_tile_cache.lat_max);
  TerrainInfo.BottomRight.Latitude =
    Angle::degrees((fixed)raster_tile_cache.lat_min);
  TerrainInfo.Columns = raster_tile_cache.GetWidth();
  TerrainInfo.Rows = raster_tile_cache.GetHeight();
  TerrainInfo.StepSize = Angle::degrees((fixed)(raster_tile_cache.lon_max -
                                                raster_tile_cache.lon_min)
                                        / raster_tile_cache.GetWidth());
}

void RasterMapJPG2000::SetViewCenter(const GEOPOINT &location)
{
  Poco::ScopedRWLock protect(lock, true);
  _SetViewCenter(location);
}

void RasterMapJPG2000::_SetViewCenter(const GEOPOINT &location)
{
  int x = 0, y = 0; /* initalize to work around bogus gcc warning */
  bool do_poll = false;
  if (raster_tile_cache.GetInitialised()) {
    do_poll = true;
    x = lround((location.Longitude-TerrainInfo.TopLeft.Longitude).value_native()*TerrainInfo.Columns
                   /(TerrainInfo.BottomRight.Longitude-TerrainInfo.TopLeft.Longitude).value_native());
    y = lround((TerrainInfo.TopLeft.Latitude-location.Latitude).value_native()*TerrainInfo.Rows
                   /(TerrainInfo.TopLeft.Latitude-TerrainInfo.BottomRight.Latitude).value_native());
    TriggerJPGReload |= raster_tile_cache.PollTiles(x, y);
  }
  if (TriggerJPGReload) {
    _ReloadJPG2000();
    if (do_poll) {
      raster_tile_cache.PollTiles(x, y);
    }
  }
}

bool
RasterMapJPG2000::Open(const char *zfilename)
{
  strcpy(jp2_filename,zfilename);

  // force first-time load
  FullJPGReload = true;

  _ReloadJPG2000();

  terrain_valid = raster_tile_cache.GetInitialised();
  if (!terrain_valid)
    raster_tile_cache.Reset();

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
