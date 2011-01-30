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

#include <algorithm>
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

bool
RasterMap::FirstIntersection(const GeoPoint &origin, const short h_origin,
                             const GeoPoint &destination, const short h_destination,
                             const short h_virt, const short h_ceiling,
                             const short h_safety,
                             GeoPoint& intx, short &h) const
{
  std::pair<unsigned, unsigned> c_origin = projection.project(origin);
  std::pair<unsigned, unsigned> c_destination = projection.project(destination);
  c_origin.first = (c_origin.first)>>8;
  c_origin.second = (c_origin.second)>>8;
  c_destination.first = (c_destination.first)>>8;
  c_destination.second = (c_destination.second)>>8;
  const long c_diff = abs((int)c_origin.first-(int)c_destination.first)+
    abs((int)c_origin.second-(int)c_destination.second);
  const bool can_climb = (h_destination< h_virt);

  intx = destination; h = h_destination; // fallback, pass
  if (c_diff==0) {
    return false; // no distance
  }

  const long slope_fact = lround((((long)h_virt)<<RASTER_SLOPE_FACT)/c_diff);
  const short vh_origin = std::max(h_origin, (short)(h_destination-
                                                     ((c_diff*slope_fact)>>RASTER_SLOPE_FACT)));

  std::pair<unsigned, unsigned> c_int= c_destination;
  if (raster_tile_cache.FirstIntersection(c_origin.first, c_origin.second,
                                          c_destination.first, c_destination.second,
                                          vh_origin, h_destination,
                                          slope_fact, h_ceiling, h_safety,
                                          c_int.first, c_int.second, h,
                                          can_climb)) {
    bool changed = (c_int != c_destination) || ((h > h_destination)&&(c_int == c_destination));
    if (changed) {
      c_int.first = (c_int.first<<8);
      c_int.second = (c_int.second<<8);
      intx = projection.unproject(c_int);
      assert(h>= h_origin);
    }
    return changed;
  } else {
    return false;
  }
}

GeoPoint
RasterMap::Intersection(const GeoPoint& origin, const short h_origin,
                        const GeoPoint& destination) const
{
  std::pair<unsigned, unsigned> c_origin = projection.project(origin);
  std::pair<unsigned, unsigned> c_destination = projection.project(destination);
  c_origin.first = (c_origin.first)>>8;
  c_origin.second = (c_origin.second)>>8;
  c_destination.first = (c_destination.first)>>8;
  c_destination.second = (c_destination.second)>>8;
  const long c_diff = abs((int)c_origin.first-(int)c_destination.first)+
    abs((int)c_origin.second-(int)c_destination.second);
  if (c_diff==0) {
    return destination; // no distance
  }
  const long slope_fact = lround((((long)h_origin)<<RASTER_SLOPE_FACT)/c_diff);

  std::pair<unsigned, unsigned> c_int;
  raster_tile_cache.Intersection(c_origin.first, c_origin.second,
                                 c_destination.first, c_destination.second,
                                 h_origin, slope_fact,
                                 c_int.first, c_int.second);

  c_int.first = (c_int.first<<8);
  c_int.second = (c_int.second<<8);
  return projection.unproject(c_int);
}
