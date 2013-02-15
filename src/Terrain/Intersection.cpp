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

#include "RasterTileCache.hpp"
#include "Terrain/RasterLocation.hpp"

#include <string.h>
#include <stdlib.h>
#include <algorithm>

//#define DEBUG_TILE
#ifdef DEBUG_TILE
#include <stdio.h>
#endif

bool
RasterTileCache::FirstIntersection(int x0, int y0,
                                   int x1, int y1,
                                   short h_origin,
                                   short h_dest,
                                   const int slope_fact, const short h_ceiling,
                                   const short h_safety,
                                   unsigned& _x, unsigned& _y, short &_h,
                                   const bool can_climb) const
{
  if (((unsigned)x0 >= width) || ((unsigned)y0 >= height))
    // origin is outside overall bounds
    return false;

  h_origin = std::max(h_origin, GetFieldDirect(x0, y0).first);
  h_dest = std::max(h_dest, h_origin);

  // line algorithm parameters
  const int dx = abs(x1-x0);
  const int dy = abs(y1-y0);
  int err = dx-dy;
  const int sx = (x0 < x1)? 1: -1;
  const int sy = (y0 < y1)? 1: -1;

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
    _x = (unsigned)x0;
    _y = (unsigned)y0;
    _h = h_origin;
    return true;
  }

#ifdef DEBUG_TILE
  printf("# fint width %d height %d\n", width, height);
#endif
  short h_terrain = 1;

  unsigned x_int= x0, y_int= y0;
  short h_int= h_origin;
  // location of last point within ceiling limit that doesnt intersect
  unsigned last_clear_x = x0;
  unsigned last_clear_y = y0;
  short last_clear_h = h_origin;

  while (h_terrain>=0) {

    if (!step_counter) {

      if (x_int >= width || y_int >= height)
        break; // outside bounds

      const auto field_direct = GetFieldDirect(x_int, y_int);
      h_terrain = field_direct.first + h_safety;
      step_counter = field_direct.second ? step_fine : step_coarse;

      // calculate height of glide so far
      const short dh = (short)((total_steps*slope_fact)>>RASTER_SLOPE_FACT);

      // current aircraft height
      h_int = dh + h_origin;
      if (can_climb) {
        h_int = std::min(h_int, h_dest);
      }

#ifdef DEBUG_TILE
      printf("%d %d %d %d %d # fint\n", x_int, y_int, h_int, h_terrain, h_ceiling);
#endif

      // this point has intersected if aircraft is below terrain height
      const bool this_intersecting = (h_int< h_terrain);

      if (this_intersecting) {
        intersect_counter = 1;

        // when intersecting, consider origin to have started higher
        const short h_jump = h_terrain - h_int;
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
        _x = last_clear_x;
        _y = last_clear_y;
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
            _x = x_int;
            _y = y_int;
            _h = h_int;
            return true;
          }
        } else {
          last_clear_x = x_int;
          last_clear_y = y_int;
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
      x_int += sx;
      if (step_counter)
        step_counter--;
      total_steps++;
    }
    if (e2 < dx) {
      err += dx;
      y_int += sy;
      if (step_counter)
        step_counter--;
      total_steps++;
    }
  }

  // early exit due to inability to find clearance after intersecting
  if (intersect_counter) {
    _x = last_clear_x;
    _y = last_clear_y;
    _h = last_clear_h;
#ifdef DEBUG_TILE
    printf("# fint early exit\n");
#endif
    return true;
  }
  return false;
}

inline std::pair<short, bool>
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

RasterLocation
RasterTileCache::Intersection(int x0, int y0,
                              int x1, int y1,
                              short h_origin,
                              const int slope_fact) const
{
  unsigned _x = (unsigned)x0;
  unsigned _y = (unsigned)y0;

  if ((_x >= width) || (_y >= height)) {
    // origin is outside overall bounds
    return RasterLocation(_x, _y);
  }

  // line algorithm parameters
  const int dx = abs(x1-x0);
  const int dy = abs(y1-y0);
  int err = dx-dy;
  const int sx = (x0 < x1)? 1: -1;
  const int sy = (y0 < y1)? 1: -1;

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

  short h_int= h_origin;
  short h_terrain = 1;

  unsigned last_clear_x = _x;
  unsigned last_clear_y = _y;
  short last_clear_h = h_int;

  while (h_terrain>=0) {

    if (!step_counter) {

      if ((_x >= width) || (_y >= height))
        break; // outside bounds

      const auto field_direct = GetFieldDirect(_x, _y);
      h_terrain = field_direct.first;
      step_counter = field_direct.second ? step_fine : step_coarse;

      // calculate height of glide so far
      const short dh = (short)((total_steps*slope_fact)>>RASTER_SLOPE_FACT);

      // current aircraft height
      h_int = h_origin-dh;

      if (h_int < h_terrain) {
        if (refine_step<3) // can't refine any further
          return RasterLocation(last_clear_x, last_clear_y);

        // refine solution
        return Intersection(last_clear_x, last_clear_y, _x, _y, last_clear_h, slope_fact);
      }

      if (h_int <= 0) 
        break; // reached max range

      last_clear_x = _x;
      last_clear_y = _y;
      last_clear_h = h_int;
    }

    if (total_steps > max_steps)
      break;

    const int e2 = 2*err;
    if (e2 > -dy) {
      err -= dy;
      _x += sx;
      if (step_counter>0)
        step_counter--;
      total_steps++;
    }
    if (e2 < dx) {
      err += dx;
      _y += sy;
      if (step_counter>0)
        step_counter--;
      total_steps++;
    }
  }

  // if we reached invalid terrain, assume we can hit MSL
  return RasterLocation(x1, y1);
}
