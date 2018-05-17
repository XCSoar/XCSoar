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

#include "Terrain/RasterTileCache.hpp"
#include "Terrain/RasterLocation.hpp"

#include <stdlib.h>

struct GridLocation : public RasterLocation {
  unsigned short tile_x, tile_y;
  unsigned remainder_x, remainder_y;
  unsigned index;

  GridLocation(const RasterLocation &other,
               unsigned tile_width, unsigned tile_height,
               unsigned _index)
    :RasterLocation(other),
     tile_x(other.x / tile_width), tile_y(other.y / tile_height),
     remainder_x(other.x % tile_width),
     remainder_y(other.y % tile_height),
     index(_index) {}
};

struct GridRay {
  unsigned tile_width, tile_height;

  GridLocation start, end;

  int delta_x, delta_y;

  unsigned size;

  GridRay(unsigned _tile_width, unsigned _tile_height,
          const RasterLocation &_start, const RasterLocation &_end,
          unsigned _size)
    :tile_width(_tile_width), tile_height(_tile_height),
     start(_start, tile_width, tile_height, 0),
     end(_end, tile_width, tile_height, _size),
     delta_x(end.x - start.x), delta_y(end.y - start.y),
     size(_size) {}
};

gcc_pure
static int
ScaleX(const GridRay &ray, int x, int range)
{
  assert(ray.delta_x != 0);

  return (int64_t)(x - (int)ray.start.x) * (int64_t)range / ray.delta_x;
}

gcc_pure
static int
YAtX(const GridRay &ray, int x)
{
  assert(ray.delta_x != 0);

  return (int)ray.start.y + ScaleX(ray, x, ray.delta_y);
}

gcc_pure
static unsigned
IndexAtX(const GridRay &ray, int x)
{
  assert(ray.delta_x != 0);

  return ScaleX(ray, x, ray.size);
}

gcc_pure
static GridLocation
NextRightGridIntersection(const GridRay &ray,
                          const GridLocation &current)
{
  assert(ray.delta_x > 0);

  if (current.tile_x == ray.end.tile_x)
    return ray.end;

  assert(current.tile_x < ray.end.tile_x);
  assert(current.x < ray.end.x);

  int x = (current.tile_x + 1) * ray.tile_width;
  int y = YAtX(ray, x);
  assert(y >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_width, ray.tile_height,
                      IndexAtX(ray, x));
}

gcc_pure
static GridLocation
NextLeftGridIntersection(const GridRay &ray, GridLocation current)
{
  assert(ray.delta_x < 0);

  if (current.tile_x == ray.end.tile_x)
    return ray.end;

  assert(current.tile_x > ray.end.tile_x);
  assert(current.x > ray.end.x);

  unsigned tile_x = current.tile_x;
  if (current.remainder_x == 0) {
    --tile_x;
    if (tile_x == ray.end.tile_x)
      return ray.end;
  }

  int x = tile_x * ray.tile_width;
  int y = YAtX(ray, x);
  assert(y >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_width, ray.tile_height,
                      IndexAtX(ray, x));
}

gcc_pure
static GridLocation
NextHorizontalGridIntersection(const GridRay &ray,
                               const GridLocation &current)
{
  if (ray.delta_x > 0)
    return NextRightGridIntersection(ray, current);
  else if (ray.delta_x < 0)
    return NextLeftGridIntersection(ray, current);
  else
    return ray.end;
}

gcc_pure
static int
ScaleY(const GridRay &ray, int y, int range)
{
  assert(ray.delta_y != 0);

  return (int64_t)(y - (int)ray.start.y) * (int64_t)range / ray.delta_y;
}

gcc_pure
static int
XAtY(const GridRay &ray, int y)
{
  assert(ray.delta_y != 0);

  return (int)ray.start.x + ScaleY(ray, y, ray.delta_x);
}

gcc_pure
static unsigned
IndexAtY(const GridRay &ray, int y)
{
  assert(ray.delta_y != 0);

  return ScaleY(ray, y, ray.size);
}

gcc_pure
static GridLocation
NextBottomGridIntersection(const GridRay &ray,
                           const GridLocation &current)
{
  assert(ray.delta_y > 0);

  if (current.tile_y == ray.end.tile_y)
    return ray.end;

  assert(current.tile_y < ray.end.tile_y);
  assert(current.y < ray.end.y);

  int y = (current.tile_y + 1) * ray.tile_height;
  int x = XAtY(ray, y);
  assert(x >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_width, ray.tile_height,
                      IndexAtY(ray, y));
}

gcc_pure
static GridLocation
NextTopGridIntersection(const GridRay &ray, GridLocation current)
{
  assert(ray.delta_y < 0);

  if (current.tile_y == ray.end.tile_y)
    return ray.end;

  assert(current.tile_y > ray.end.tile_y);
  assert(current.y > ray.end.y);

  unsigned tile_y = current.tile_y;
  if (current.remainder_y == 0) {
    --tile_y;
    if (tile_y == ray.end.tile_y)
      return ray.end;
  }

  int y = tile_y * ray.tile_height;
  int x = XAtY(ray, y);
  assert(x >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_width, ray.tile_height,
                      IndexAtY(ray, y));
}

gcc_pure
static GridLocation
NextVerticalGridIntersection(const GridRay &ray,
                             const GridLocation &current)
{
  assert(ray.start.index == 0);
  assert(ray.end.index == ray.size);

  if (ray.delta_y > 0)
    return NextBottomGridIntersection(ray, current);
  else if (ray.delta_y < 0)
    return NextTopGridIntersection(ray, current);
  else
    return ray.end;
}

gcc_pure
static GridLocation
NextGridIntersection(const GridRay &ray,
                     const GridLocation &current)
{
  assert(ray.start.index == 0);
  assert(ray.end.index > 0);

  GridLocation h = NextHorizontalGridIntersection(ray, current);
  GridLocation v = NextVerticalGridIntersection(ray, current);

  return abs((int)(h.x - ray.start.x)) + abs((int)(h.y - ray.start.y)) <
    abs((int)(v.x - ray.start.x)) + abs((int)(v.y - ray.start.y))
    ? h : v;
}

inline void
RasterTileCache::ScanTileLine(GridLocation start, GridLocation end,
                              TerrainHeight *buffer, unsigned size,
                              bool interpolate) const
{
  assert(end.index >= start.index);
  assert(end.index <= size);

  if (start.index == end.index)
    return;

  if (start.tile_x < end.tile_x) {
    assert(end.tile_x == start.tile_x + 1);
    assert(end.remainder_x == 0);

    --end.x;
  } else if (start.tile_x > end.tile_x) {
    assert(start.tile_x == end.tile_x + 1);
    assert(start.remainder_x == 0);

    --start.x;
    --start.tile_x;
  }

  if (start.tile_y < end.tile_y) {
    assert(end.tile_y == start.tile_y + 1);
    assert(end.remainder_y == 0);

    --end.y;
  } else if (start.tile_y > end.tile_y) {
    assert(start.tile_y == end.tile_y + 1);
    assert(start.remainder_y == 0);

    --start.y;
    --start.tile_y;
  }

  const RasterTile &tile = tiles.Get(start.tile_x, start.tile_y);
  if (tile.IsEnabled())
    tile.ScanLine(start.x, start.y, end.x, end.y,
                  buffer + start.index, end.index - start.index,
                  interpolate);
  else
    /* need range checking in the overview buffer because its size may
       be rounded down, and then the "fine" location may exceed its
       bounds */
    overview.ScanLineChecked(start.x >> OVERVIEW_BITS,
                             start.y >> OVERVIEW_BITS,
                             end.x >> OVERVIEW_BITS, end.y >> OVERVIEW_BITS,
                             buffer + start.index, end.index - start.index,
                             interpolate);
}

void
RasterTileCache::ScanLine(const RasterLocation _start,
                          const RasterLocation _end,
                          TerrainHeight *buffer, unsigned size,
                          bool interpolate) const
{
  assert(_start.x < GetFineWidth());
  assert(_start.y < GetFineHeight());
  assert(_end.x < GetFineWidth());
  assert(_end.y < GetFineHeight());
  assert(size >= 2);

  const GridRay ray(GetFineTileWidth(), GetFineTileHeight(),
                    _start, _end, size);
  assert(ray.size == size);
  assert(ray.start.index == 0);
  assert(ray.end.index == size);

  GridLocation current = ray.start;
  while (current.index < size) {
    GridLocation next = NextGridIntersection(ray, current);
    ScanTileLine(current, next, buffer, size, interpolate);
    current = next;
  }
}
