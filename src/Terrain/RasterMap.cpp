/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Geo/GeoClip.hpp"
#include "IO/FileCache.hpp"
#include "Util/ConvertString.hpp"

#include <algorithm>
#include <assert.h>
#include <string.h>

static char *
ToNarrowPath(const TCHAR *src)
{
  return WideToACPConverter(src).StealDup();
}

RasterMap::RasterMap(const TCHAR *_path, const TCHAR *world_file,
                     FileCache *cache, OperationEnvironment &operation)
  :path(ToNarrowPath(_path))
{
  bool cache_loaded = false;
  if (cache != NULL) {
    /* load the cache file */
    FILE *file = cache->Load(_T("terrain"), _path);
    if (file != NULL) {
      cache_loaded = raster_tile_cache.LoadCache(file);
      fclose(file);
    }
  }

  if (!cache_loaded) {
    if (!raster_tile_cache.LoadOverview(path, world_file, operation))
      return;

    if (cache != NULL) {
      /* save the cache file */
      FILE *file = cache->Save(_T("terrain"), _path);
      if (file != NULL) {
        if (raster_tile_cache.SaveCache(file))
          cache->Commit(_T("terrain"), file);
        else
          cache->Cancel(_T("terrain"), file);
      }
    }
  }

  projection.Set(GetBounds(),
                 raster_tile_cache.GetFineWidth(),
                 raster_tile_cache.GetFineHeight());
}

RasterMap::~RasterMap() {
  free(path);
}

static unsigned
AngleToPixel(Angle value, Angle start, Angle end, unsigned width)
{
  return unsigned((value - start).Native() * width / (end - start).Native());
}

void
RasterMap::SetViewCenter(const GeoPoint &location, fixed radius)
{
  if (!raster_tile_cache.GetInitialised())
    return;

  const GeoBounds &bounds = GetBounds();

  int x = AngleToPixel(location.longitude, bounds.GetWest(), bounds.GetEast(),
                       raster_tile_cache.GetWidth());

  int y = AngleToPixel(location.latitude, bounds.GetNorth(), bounds.GetSouth(),
                       raster_tile_cache.GetHeight());

  raster_tile_cache.UpdateTiles(path, x, y,
                                projection.DistancePixelsCoarse(radius));
}

short
RasterMap::GetHeight(const GeoPoint &location) const
{
  RasterLocation pt = projection.ProjectCoarse(location);
  return raster_tile_cache.GetHeight(pt.x, pt.y);
}

short
RasterMap::GetInterpolatedHeight(const GeoPoint &location) const
{
  RasterLocation pt = projection.ProjectFine(location);
  return raster_tile_cache.GetInterpolatedHeight(pt.x, pt.y);
}

void
RasterMap::ScanLine(const GeoPoint &start, const GeoPoint &end,
                    short *buffer, unsigned size, bool interpolate) const
{
  assert(buffer != NULL);
  assert(size > 0);

  const short invalid = RasterBuffer::TERRAIN_INVALID;

  const fixed total_distance = start.Distance(end);
  if (!positive(total_distance)) {
    std::fill_n(buffer, size, invalid);
    return;
  }

  /* clip the line to the map bounds */

  GeoPoint clipped_start = start, clipped_end = end;
  const GeoClip clip(GetBounds());
  if (!clip.ClipLine(clipped_start, clipped_end)) {
    std::fill_n(buffer, size, invalid);
    return;
  }

  fixed clipped_start_distance =
    std::max(clipped_start.Distance(start), fixed(0));
  fixed clipped_end_distance =
    std::max(clipped_end.Distance(start), fixed(0));

  /* calculate the offsets of the clipped range within the buffer */

  unsigned clipped_start_offset =
    (unsigned)(size * clipped_start_distance / total_distance);
  unsigned clipped_end_offset =
    uround(size * clipped_end_distance / total_distance);
  if (clipped_end_offset > size)
    clipped_end_offset = size;
  if (clipped_start_offset + 2 > clipped_end_offset) {
    std::fill_n(buffer, size, invalid);
    return;
  }

  assert(clipped_start_offset < size);
  assert(clipped_end_offset <= size);

  /* fill the two regions which are outside the map  */

  std::fill(buffer, buffer + clipped_start_offset, invalid);
  std::fill(buffer + clipped_end_offset, buffer + size, invalid);

  /* now scan the middle part which is within the map */

  const unsigned max_x = raster_tile_cache.GetFineWidth();
  const unsigned max_y = raster_tile_cache.GetFineHeight();

  RasterLocation raster_start = projection.ProjectFine(clipped_start);
  if (raster_start.x >= max_x)
    raster_start.x = max_x - 1;
  if (raster_start.y >= max_y)
    raster_start.y = max_y - 1;

  RasterLocation raster_end = projection.ProjectFine(clipped_end);
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
RasterMap::FirstIntersection(const GeoPoint &origin, const int h_origin,
                             const GeoPoint &destination, const int h_destination,
                             const int h_virt, const int h_ceiling,
                             const int h_safety,
                             GeoPoint &intx, int &h) const
{
  const RasterLocation c_origin = projection.ProjectCoarse(origin);
  const RasterLocation c_destination = projection.ProjectCoarse(destination);
  const int c_diff = c_origin.ManhattanDistance(c_destination);
  const bool can_climb = (h_destination< h_virt);

  intx = destination; h = h_destination; // fallback, pass
  if (c_diff==0) {
    return false; // no distance
  }

  const int slope_fact = (((int)h_virt) << RASTER_SLOPE_FACT) / c_diff;
  const int vh_origin = std::max(h_origin,
                                 h_destination
                                 - ((c_diff * slope_fact) >> RASTER_SLOPE_FACT));

  RasterLocation c_int;
  if (raster_tile_cache.FirstIntersection(c_origin.x, c_origin.y,
                                          c_destination.x, c_destination.y,
                                          vh_origin, h_destination,
                                          slope_fact, h_ceiling, h_safety,
                                          c_int, h,
                                          can_climb)) {
    bool changed = c_int != c_destination ||
      (h > h_destination && c_int == c_destination);
    if (changed) {
      intx = projection.UnprojectCoarse(c_int);
      assert(h>= h_origin);
    }
    return changed;
  } else {
    return false;
  }
}

GeoPoint
RasterMap::Intersection(const GeoPoint& origin,
                        const int h_origin, const int h_glide,
                        const GeoPoint& destination) const
{
  const RasterLocation c_origin = projection.ProjectCoarse(origin);
  const RasterLocation c_destination = projection.ProjectCoarse(destination);
  const int c_diff = c_origin.ManhattanDistance(c_destination);
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

  return projection.UnprojectCoarse(c_int);
}
