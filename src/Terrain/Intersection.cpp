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

#include "RasterTileCache.hpp"
#include "Terrain/RasterLocation.hpp"

#include <stdlib.h>
#include <algorithm>

//#define DEBUG_TILE
#ifdef DEBUG_TILE
#include <stdio.h>
#endif

bool
RasterTileCache::FirstIntersection(const SignedRasterLocation origin,
                                   const SignedRasterLocation destination,
                                   int h_origin,
                                   int h_dest,
                                   const int slope_fact, const int h_ceiling,
                                   const int h_safety,
                                   RasterLocation &_location, int &_h,
                                   const bool can_climb) const
{
  RasterLocation location = origin;
  if (!IsInside(location))
    // origin is outside overall bounds
    return false;

  const TerrainHeight h_origin2 = GetFieldDirect(origin.x, origin.y).first;
  if (h_origin2.IsInvalid()) {
    _location = location;
    _h = h_origin;
    return true;
  }

  if (!h_origin2.IsSpecial())
    h_origin = std::max(h_origin, (int)h_origin2.GetValue());

  h_dest = std::max(h_dest, h_origin);

  // line algorithm parameters
  const int dx = abs(destination.x - origin.x);
  const int dy = abs(destination.y - origin.y);
  int err = dx-dy;
  const int sx = origin.x < destination.x ? 1 : -1;
  const int sy = origin.y < destination.y ? 1 : -1;

  // max number of steps to walk
  const int max_steps = (dx+dy);
  // calculate number of fine steps to produce a step on the overview field
  const int step_fine = std::max(1, max_steps >> INTERSECT_BITS);
  // number of steps for update to the overview map
  const int step_coarse = std::max(1<< OVERVIEW_BITS, step_fine);

  // number of steps to be cleared after climbing over obstruction
  const int intersect_steps = 32;

  // counter for steps to reach next position to be checked on the field.
  unsigned step_counter = 0;
  // total counter of fine steps
  int total_steps = 0;

  // number of steps since intersection
  int intersect_counter = 0;

#ifdef DEBUG_TILE
  printf("# max steps %d\n", max_steps);
  printf("# step coarse %d\n", step_coarse);
  printf("# step fine %d\n", step_fine);
#endif

  // early exit if origin is too high (should not occur)
  if (h_origin> h_ceiling) {
#ifdef DEBUG_TILE
    printf("# fint start above ceiling %d %d\n", h_origin, h_ceiling);
#endif
    _location = location;
    _h = h_origin;
    return true;
  }

#ifdef DEBUG_TILE
  printf("# fint width %d height %d\n", width, height);
#endif

  // location of last point within ceiling limit that doesnt intersect
  RasterLocation last_clear_location = location;
  int last_clear_h = h_origin;

  while (true) {

    if (!step_counter) {

      if (!IsInside(location))
        break; // outside bounds

      const auto field_direct = GetFieldDirect(location.x, location.y);
      if (field_direct.first.IsInvalid())
        break;

      const int h_terrain = field_direct.first.GetValueOr0() + h_safety;
      step_counter = field_direct.second ? step_fine : step_coarse;

      // calculate height of glide so far
      const int dh = (total_steps * slope_fact) >> RASTER_SLOPE_FACT;

      // current aircraft height
      int h_int = dh + h_origin;
      if (can_climb) {
        h_int = std::min(h_int, h_dest);
      }

#ifdef DEBUG_TILE
      printf("%d %d %d %d %d # fint\n", location.x, location.y, h_int, h_terrain, h_ceiling);
#endif

      // this point has intersected if aircraft is below terrain height
      const bool this_intersecting = (h_int< h_terrain);

      if (this_intersecting) {
        intersect_counter = 1;

        // when intersecting, consider origin to have started higher
        const int h_jump = h_terrain - h_int;
        h_origin += h_jump;

        if (can_climb) {
          // if intersecting beyond desired destination height, allow dest height
          // to be increased
          if (h_terrain> h_dest)
            h_dest = h_terrain;
        } else {
          // if can't climb, must jump so path is pure glide
          h_dest += h_jump;
        }
        h_int = h_terrain;

      }

      if (h_int > h_ceiling) {
        _location = last_clear_location;
        _h = last_clear_h;
#ifdef DEBUG_TILE
        printf("# fint reach ceiling\n");
#endif
        return true; // reached ceiling
      }

      if (!this_intersecting) {
        if (intersect_counter) {
          intersect_counter+= step_counter;

          // was intersecting, now cleared.
          // exit with small height above terrain
#ifdef DEBUG_TILE
          printf("# fint int->clear\n");
#endif
          if (intersect_counter >= intersect_steps) {
            _location = location;
            _h = h_int;
            return true;
          }
        } else {
          last_clear_location = location;
          last_clear_h = h_int;
        }
      }
    }

    if (!intersect_counter && (total_steps == max_steps)) {
#ifdef DEBUG_TILE
      printf("# fint cleared\n");
#endif
      return false;
    }

    const int e2 = 2*err;
    if (e2 > -dy) {
      err -= dy;
      location.x += sx;
      if (step_counter)
        step_counter--;
      total_steps++;
    }
    if (e2 < dx) {
      err += dx;
      location.y += sy;
      if (step_counter)
        step_counter--;
      total_steps++;
    }
  }

  // early exit due to inability to find clearance after intersecting
  if (intersect_counter) {
    _location = last_clear_location;
    _h = last_clear_h;
#ifdef DEBUG_TILE
    printf("# fint early exit\n");
#endif
    return true;
  }
  return false;
}

inline std::pair<TerrainHeight, bool>
RasterTileCache::GetFieldDirect(const unsigned px, const unsigned py) const
{
  assert(px < width);
  assert(py < height);

  const RasterTile &tile = tiles.Get(px / tile_width, py / tile_height);
  if (tile.IsEnabled())
    return std::make_pair(tile.GetHeight(px, py), true);

  // still not found, so go to overview

  // The overview might not cover the whole tile, if width or height are not
  // a multiple of 2^OVERVIEW_BITS.
  unsigned x_overview = px >> OVERVIEW_BITS;
  unsigned y_overview = py >> OVERVIEW_BITS;
  assert(x_overview <= overview.GetWidth());
  assert(y_overview <= overview.GetHeight());

  if (x_overview == overview.GetWidth())
    x_overview--;
  if (y_overview == overview.GetHeight())
    y_overview--;

  return std::make_pair(overview.Get(x_overview, y_overview), false);
}

SignedRasterLocation
RasterTileCache::Intersection(const SignedRasterLocation origin,
                              const SignedRasterLocation destination,
                              const int h_origin,
                              const int slope_fact,
                              const int height_floor) const
{
  SignedRasterLocation location = origin;

  if (!IsInside(location))
    // origin is outside overall bounds
    return {-1, -1};

  // line algorithm parameters
  const int dx = abs(destination.x - origin.x);
  const int dy = abs(destination.y - origin.y);
  int err = dx-dy;
  const int sx = origin.x < destination.x ? 1 : -1;
  const int sy = origin.y < destination.y ? 1 : -1;

  // max number of steps to walk
  const int max_steps = (dx+dy);
  // calculate number of fine steps to produce a step on the overview field

  // step size at selected refinement level
  const int refine_step = max_steps >> 5;

  // number of steps for update to the fine map
  const int step_fine = std::max(1, refine_step);
  // number of steps for update to the overview map
  const int step_coarse = std::max(1<< OVERVIEW_BITS, step_fine);

  // counter for steps to reach next position to be checked on the field.
  unsigned step_counter = 0;
  // total counter of fine steps
  int total_steps = 0;

#ifdef DEBUG_TILE
  printf("# max steps %d\n", max_steps);
  printf("# step coarse %d\n", step_coarse);
  printf("# step fine %d\n", step_fine);
#endif

  RasterLocation last_clear_location = location;
  int last_clear_h = h_origin;

  while (true) {

    if (!step_counter) {

      if (!IsInside(location))
        break;

      const auto field_direct = GetFieldDirect(location.x, location.y);
      if (field_direct.first.IsInvalid())
        break;

      const int h_terrain = field_direct.first.GetValueOr0();
      step_counter = field_direct.second ? step_fine : step_coarse;

      // calculate height of glide so far
      const int dh = (total_steps * slope_fact) >> RASTER_SLOPE_FACT;

      // current aircraft height
      const int h_int = h_origin - dh;

      if (h_int < std::max(h_terrain, height_floor)) {
        if (refine_step<3) // can't refine any further
          return RasterLocation(last_clear_location.x, last_clear_location.y);

        // refine solution
        return Intersection(last_clear_location, location,
                            last_clear_h, slope_fact, height_floor);
      }

      if (h_int <= 0)
        break; // reached max range

      last_clear_location = location;
      last_clear_h = h_int;
    }

    if (total_steps > max_steps)
      break;

    const int e2 = 2*err;
    if (e2 > -dy) {
      err -= dy;
      location.x += sx;
      if (step_counter>0)
        step_counter--;
      total_steps++;
    }
    if (e2 < dx) {
      err += dx;
      location.y += sy;
      if (step_counter>0)
        step_counter--;
      total_steps++;
    }
  }

  // if we reached invalid terrain, assume we can hit MSL
  return {-1, -1};
}
