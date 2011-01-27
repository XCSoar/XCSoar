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

#include "Terrain/RasterMap.hpp"
#include "Math/Earth.hpp"
#include "OS/PathName.hpp"
#include "IO/FileCache.hpp"

#include <assert.h>
#include <string.h>

RasterMap::RasterMap(const TCHAR *_path, const TCHAR *world_file,
                     FileCache *cache)
  :path(strdup(NarrowPathName(_path)))
{
  bool cache_loaded = false;
  if (cache != NULL) {
    /* load the cache file */
    FILE *file = cache->load(_T("terrain"), _path);
    if (file != NULL) {
      cache_loaded = raster_tile_cache.LoadCache(file);
      fclose(file);
    }
  }

  if (!cache_loaded) {
    if (!raster_tile_cache.LoadOverview(path, world_file))
      return;

    if (cache != NULL) {
      /* save the cache file */
      FILE *file = cache->save(_T("terrain"), _path);
      if (file != NULL) {
        if (raster_tile_cache.SaveCache(file))
          cache->commit(_T("terrain"), file);
        else
          cache->cancel(_T("terrain"), file);
      }
    }
  }

  projection.set(raster_tile_cache.GetBounds(),
                 raster_tile_cache.GetWidth() * 256,
                 raster_tile_cache.GetHeight() * 256);
}

RasterMap::~RasterMap() {
  free(path);
}

static unsigned
angle_to_pixel(Angle value, Angle start, Angle end, unsigned width)
{
  return (value - start).value_native() * width / (end - start).value_native();
}

void
RasterMap::SetViewCenter(const GeoPoint &location, fixed radius)
{
  if (!raster_tile_cache.GetInitialised())
    return;

  const GeoBounds &bounds = raster_tile_cache.GetBounds();

  int x = angle_to_pixel(location.Longitude, bounds.west, bounds.east,
                         raster_tile_cache.GetWidth());

  int y = angle_to_pixel(location.Latitude, bounds.north, bounds.south,
                         raster_tile_cache.GetHeight());

  raster_tile_cache.UpdateTiles(path, x, y,
                                projection.distance_pixels(radius) / 256);
}

short
RasterMap::GetField(const GeoPoint &location) const
{
  std::pair<unsigned, unsigned> xy = projection.project(location);
  return raster_tile_cache.GetField(xy.first, xy.second);
}

short
RasterMap::GetFieldInterpolated(const GeoPoint &location) const
{
  std::pair<unsigned, unsigned> xy = projection.project(location);
  return raster_tile_cache.GetFieldInterpolated(xy.first, xy.second);
}
