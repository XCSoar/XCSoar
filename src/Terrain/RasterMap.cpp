/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Math/Util.hpp"

#include <algorithm>
#include <assert.h>

void
RasterMap::UpdateProjection()
{
  projection.Set(GetBounds(),
                 raster_tile_cache.GetFineWidth(),
                 raster_tile_cache.GetFineHeight());
}

bool
RasterMap::LoadCache(FILE *file)
{
  bool success = raster_tile_cache.LoadCache(file);
  if (success)
    UpdateProjection();

  return success;
}

TerrainHeight
RasterMap::GetHeight(const GeoPoint &location) const
{
  const auto pt = projection.ProjectCoarse(location);
  return raster_tile_cache.GetHeight(pt.x, pt.y);
}

TerrainHeight
RasterMap::GetInterpolatedHeight(const GeoPoint &location) const
{
  const auto pt = projection.ProjectFine(location);
  return raster_tile_cache.GetInterpolatedHeight(pt.x, pt.y);
}

void
RasterMap::ScanLine(const GeoPoint &start, const GeoPoint &end,
                    TerrainHeight *buffer, unsigned size,
                    bool interpolate) const
{
  assert(buffer != nullptr);
  assert(size > 0);

  constexpr TerrainHeight invalid = TerrainHeight::Invalid();

  const double total_distance = start.DistanceS(end);
  if (total_distance <= 0) {
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

  double clipped_start_distance =
    std::max(clipped_start.DistanceS(start), 0.);
  double clipped_end_distance =
    std::max(clipped_end.DistanceS(start), 0.);

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
  const auto c_origin = projection.ProjectCoarseRound(origin);
  const auto c_destination = projection.ProjectCoarseRound(destination);
  const int c_diff = ManhattanDistance(c_origin, c_destination);
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
  if (raster_tile_cache.FirstIntersection(c_origin, c_destination,
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
                        const GeoPoint& destination,
                        const int height_floor) const
{
  const auto c_origin = projection.ProjectCoarseRound(origin);
  const auto c_destination = projection.ProjectCoarseRound(destination);
  const int c_diff = ManhattanDistance(c_origin, c_destination);
  if (c_diff == 0)
    return GeoPoint::Invalid();

  const int slope_fact = (((int)h_glide) << RASTER_SLOPE_FACT) / c_diff;

  auto c_int =
    raster_tile_cache.Intersection(c_origin, c_destination,
                                   h_origin, slope_fact, height_floor);
  if (c_int.x < 0)
    return GeoPoint::Invalid();

  return projection.UnprojectCoarse(c_int);
}
