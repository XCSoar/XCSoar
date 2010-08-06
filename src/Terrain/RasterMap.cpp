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

#include "Terrain/RasterMap.hpp"
#include "Math/Earth.hpp"
#include "jasper/jasper.h"

#include <assert.h>
#include <string.h>

int RasterMap::ref_count = 0;

RasterMap::RasterMap(const char *_path)
  :path(strdup(_path))
{
  if (ref_count==0) {
    jas_init();
  }
  ref_count++;

  _ReloadJPG2000();

  if (!raster_tile_cache.GetInitialised())
    raster_tile_cache.Reset();
}

RasterMap::~RasterMap() {
  ref_count--;
  if (ref_count==0) {
    jas_cleanup();
  }

  free(path);
}

static unsigned
angle_to_pixel(Angle value, Angle start, Angle end, unsigned width)
{
  return (value - start).value_native() * width / (end - start).value_native();
}

void
RasterMap::SetViewCenter(const GEOPOINT &location)
{
  if (!raster_tile_cache.GetInitialised())
    return;

  int x = angle_to_pixel(location.Longitude, bounds.west, bounds.east,
                         raster_tile_cache.GetWidth());

  int y = angle_to_pixel(location.Latitude, bounds.north, bounds.south,
                         raster_tile_cache.GetHeight());

  if (raster_tile_cache.PollTiles(x, y)) {
    _ReloadJPG2000();
    raster_tile_cache.PollTiles(x, y);
  }
}

// accurate method
int
RasterMap::GetEffectivePixelSize(fixed &pixel_D,
                                 const GEOPOINT &location) const
{
  fixed terrain_step_x, terrain_step_y;
  Angle step_size = TerrainInfo.StepSize * sqrt(fixed_two); 

  if (negative(pixel_D) || (step_size.sign()==0)) {
    pixel_D = fixed_one;
    return 1;
  }
  GEOPOINT dloc;
  
  // how many steps are in the pixel size
  dloc = location; dloc.Latitude += step_size;
  terrain_step_x = Distance(location, dloc);

  dloc = location; dloc.Longitude += step_size;
  terrain_step_y = Distance(location, dloc);

  fixed rfact = max(terrain_step_x, terrain_step_y) / pixel_D;

  int epx = (int)(max(fixed_one, ceil(rfact)));
  //  *pixel_D = (*pixel_D)*rfact/epx;
  return epx;
}

short
RasterMap::GetField(const GEOPOINT &location)
{
  return raster_tile_cache.GetField((int)(location.Longitude.value_native() *
                             rounding.fXroundingFine) - rounding.xlleft,
                       rounding.xlltop -
                       (int)(location.Latitude.value_native() *
                             rounding.fYroundingFine));
}

void
RasterMap::_ReloadJPG2000()
{
  raster_tile_cache.LoadJPG2000(path);

  if (!raster_tile_cache.GetInitialised())
    return;

  bounds.west = Angle::degrees((fixed)raster_tile_cache.lon_min);
  bounds.east = Angle::degrees((fixed)raster_tile_cache.lon_max);
  bounds.north = Angle::degrees((fixed)raster_tile_cache.lat_max);
  bounds.south = Angle::degrees((fixed)raster_tile_cache.lat_min);

  TerrainInfo.StepSize = Angle::degrees((fixed)(raster_tile_cache.lon_max -
                                                raster_tile_cache.lon_min)
                                        / raster_tile_cache.GetWidth());

  // use double here for maximum accuracy, since we are dealing with
  // numbers close to the lower range of the fixed type

  const double fx = (double)TerrainInfo.StepSize.value_native();
  const double fy = (double)TerrainInfo.StepSize.value_native();

  rounding.fXroundingFine = fixed(256.0/fx);
  rounding.fYroundingFine = fixed(256.0/fy);

  rounding.xlleft = (int)(bounds.west.value_native() * rounding.fXroundingFine) + 128;
  rounding.xlltop = (int)(bounds.north.value_native() * rounding.fYroundingFine) - 128;
}

RasterMap *
RasterMap::LoadFile(const char *path)
{
  RasterMap *map = new RasterMap(path);
  if (map == NULL)
    return NULL;

  if (!map->isMapLoaded()) {
    delete map;
    return NULL;
  }

  return map;
}
