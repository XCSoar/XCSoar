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
#include "Geo/GeoClip.hpp"
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
RasterMap::GetHeight(const GeoPoint &location) const
{
  RasterLocation pt = projection.project(location) >> 8;
  return raster_tile_cache.GetHeight(pt.x, pt.y);
}

short
RasterMap::GetInterpolatedHeight(const GeoPoint &location) const
{
  RasterLocation pt = projection.project(location);
  return raster_tile_cache.GetInterpolatedHeight(pt.x, pt.y);
}

void
RasterMap::ScanLine(const GeoPoint &start, const GeoPoint &end,
                    short *buffer, unsigned size, bool interpolate) const
{
  assert(buffer != NULL);
  assert(size > 0);

  const short invalid = RasterBuffer::TERRAIN_INVALID;

  const fixed total_distance = start.distance(end);
  if (!positive(total_distance)) {
    std::fill(buffer, buffer + size, invalid);
    return;
  }

  /* clip the line to the map bounds */

  GeoPoint clipped_start = start, clipped_end = end;
  const GeoClip clip(raster_tile_cache.GetBounds());
  if (!clip.clip_line(clipped_start, clipped_end)) {
    std::fill(buffer, buffer + size, invalid);
    return;
  }

  fixed clipped_start_distance =
    std::max(clipped_start.distance(start), fixed_zero);
  fixed clipped_end_distance =
    std::max(clipped_end.distance(start), fixed_zero);

  /* calculate the offsets of the clipped range within the buffer */

  unsigned clipped_start_offset =
    (unsigned)(size * clipped_start_distance / total_distance);
  unsigned clipped_end_offset =
    uround(size * clipped_end_distance / total_distance);
  if (clipped_end_offset > size)
    clipped_end_offset = size;
  if (clipped_start_offset + 2 > clipped_end_offset) {
    std::fill(buffer, buffer + size, invalid);
    return;
  }

  assert(clipped_start_offset < size);
  assert(clipped_end_offset <= size);

  /* fill the two regions which are outside the map  */

  std::fill(buffer, buffer + clipped_start_offset, invalid);
  std::fill(buffer + clipped_end_offset, buffer + size, invalid);

  /* now scan the middle part which is within the map */

  const unsigned max_x =
    raster_tile_cache.GetWidth() << RasterTileCache::SUBPIXEL_BITS;
  const unsigned max_y =
    raster_tile_cache.GetHeight() << RasterTileCache::SUBPIXEL_BITS;

  RasterLocation raster_start = projection.project(clipped_start);
  if (raster_start.x >= max_x)
    raster_start.x = max_x - 1;
  if (raster_start.y >= max_y)
    raster_start.y = max_y - 1;

  RasterLocation raster_end = projection.project(clipped_end);
  if (raster_end.x >= max_x)
    raster_end.x = max_x - 1;
  if (raster_end.y >= max_y)
    raster_end.y = max_y - 1;

  raster_tile_cache.ScanLine(raster_start, raster_end,
                             buffer + clipped_start_offset,
                             clipped_end_offset - clipped_start_offset,
                             interpolate);
}

bool
RasterMap::FirstIntersection(const GeoPoint &origin, const short h_origin,
                             const GeoPoint &destination, const short h_destination,
                             const short h_virt, const short h_ceiling,
                             const short h_safety,
                             GeoPoint& intx, short &h) const
{
  const RasterLocation c_origin = projection.project_coarse(origin);
  const RasterLocation c_destination = projection.project_coarse(destination);
  const int c_diff = c_origin.manhattan_distance(c_destination);
  const bool can_climb = (h_destination< h_virt);

  intx = destination; h = h_destination; // fallback, pass
  if (c_diff==0) {
    return false; // no distance
  }

  const int slope_fact = (((int)h_virt) << RASTER_SLOPE_FACT) / c_diff;
  const short vh_origin = std::max(h_origin, (short)(h_destination-
                                                     ((c_diff*slope_fact)>>RASTER_SLOPE_FACT)));

  RasterLocation c_int = c_destination;
  if (raster_tile_cache.FirstIntersection(c_origin.x, c_origin.y,
                                          c_destination.x, c_destination.y,
                                          vh_origin, h_destination,
                                          slope_fact, h_ceiling, h_safety,
                                          c_int.x, c_int.y, h,
                                          can_climb)) {
    bool changed = c_int != c_destination ||
      (h > h_destination && c_int == c_destination);
    if (changed) {
      intx = projection.unproject_coarse(c_int);
      assert(h>= h_origin);
    }
    return changed;
  } else {
    return false;
  }
}

GeoPoint
RasterMap::Intersection(const GeoPoint& origin,
                        const short h_origin,
                        const short h_glide,
                        const GeoPoint& destination) const
{
  const RasterLocation c_origin = projection.project_coarse(origin);
  const RasterLocation c_destination = projection.project_coarse(destination);
  const int c_diff = c_origin.manhattan_distance(c_destination);
  if (c_diff==0) {
    return destination; // no distance
  }
  const int slope_fact = (((int)h_glide) << RASTER_SLOPE_FACT) / c_diff;

  RasterLocation c_int =
    raster_tile_cache.Intersection(c_origin.x, c_origin.y,
                                   c_destination.x, c_destination.y,
                                   h_origin, slope_fact);

  if (c_int == c_destination) // made it to grid location, return exact location
                              // of destination
    return destination;

  return projection.unproject_coarse(c_int);
}
