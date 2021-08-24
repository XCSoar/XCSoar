/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

/**
 * A #RasterLocation with some cached computations.  The
 * #RasterLocation base holds the linear subpixel coordinates within
 * the #RasterMap.
 */
struct GridLocation : public RasterLocation {
  /**
   * The #RasterTile within the #RasterMap.
   */
  Point2D<uint_least16_t> tile;

  /**
   * The position within the #RasterTile.
   */
  UnsignedPoint2D remainder;

  /**
   * The index in the destination buffer which is about to be filled.
   */
  unsigned index;

  constexpr GridLocation(const RasterLocation &other,
                         UnsignedPoint2D tile_size,
                         unsigned _index) noexcept
    :RasterLocation(other),
     tile(other.x / tile_size.x, other.y / tile_size.y),
     remainder{other.x % tile_size.x, other.y % tile_size.y},
     index(_index) {}
};

struct GridRay {
  /**
   * The dimensions of one tile (in subpixels).
   */
  UnsignedPoint2D tile_size;

  /**
   * Start and end of the ray.
   */
  GridLocation start, end;

  /**
   * The difference vector between "start" and "end" (in subpixels).
   */
  IntPoint2D delta;

  /**
   * The size (number of #TerrainHeight elements) of the destination
   * buffer which is about to be filled.
   */
  unsigned size;

  constexpr GridRay(UnsignedPoint2D _tile_size,
                    const RasterLocation &_start, const RasterLocation &_end,
                    unsigned _size) noexcept
    :tile_size(_tile_size),
     start(_start, tile_size, 0),
     end(_end, tile_size, _size),
     delta(end.x - start.x, end.y - start.y),
     size(_size) {}
};

static constexpr int
ScaleX(const GridRay &ray, int x, int range) noexcept
{
  assert(ray.delta.x != 0);

  return (int64_t)(x - (int)ray.start.x) * (int64_t)range / ray.delta.x;
}

static constexpr int
YAtX(const GridRay &ray, int x) noexcept
{
  assert(ray.delta.x != 0);

  return (int)ray.start.y + ScaleX(ray, x, ray.delta.y);
}

static constexpr unsigned
IndexAtX(const GridRay &ray, int x) noexcept
{
  assert(ray.delta.x != 0);

  return ScaleX(ray, x, ray.size);
}

static constexpr GridLocation
NextRightGridIntersection(const GridRay &ray,
                          const GridLocation &current) noexcept
{
  assert(ray.delta.x > 0);

  if (current.tile.x == ray.end.tile.x)
    return ray.end;

  assert(current.tile.x < ray.end.tile.x);
  assert(current.x < ray.end.x);

  int x = (current.tile.x + 1) * ray.tile_size.x;
  int y = YAtX(ray, x);
  assert(y >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_size,
                      IndexAtX(ray, x));
}

static constexpr GridLocation
NextLeftGridIntersection(const GridRay &ray, GridLocation current) noexcept
{
  assert(ray.delta.x < 0);

  if (current.tile.x == ray.end.tile.x)
    return ray.end;

  assert(current.tile.x > ray.end.tile.x);
  assert(current.x > ray.end.x);

  unsigned tile_x = current.tile.x;
  if (current.remainder.x == 0) {
    --tile_x;
    if (tile_x == ray.end.tile.x)
      return ray.end;
  }

  int x = tile_x * ray.tile_size.x;
  int y = YAtX(ray, x);
  assert(y >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_size,
                      IndexAtX(ray, x));
}

static constexpr GridLocation
NextHorizontalGridIntersection(const GridRay &ray,
                               const GridLocation &current) noexcept
{
  if (ray.delta.x > 0)
    return NextRightGridIntersection(ray, current);
  else if (ray.delta.x < 0)
    return NextLeftGridIntersection(ray, current);
  else
    return ray.end;
}

static constexpr int
ScaleY(const GridRay &ray, int y, int range) noexcept
{
  assert(ray.delta.y != 0);

  return (int64_t)(y - (int)ray.start.y) * (int64_t)range / ray.delta.y;
}

static constexpr int
XAtY(const GridRay &ray, int y) noexcept
{
  assert(ray.delta.y != 0);

  return (int)ray.start.x + ScaleY(ray, y, ray.delta.x);
}

static constexpr unsigned
IndexAtY(const GridRay &ray, int y) noexcept
{
  assert(ray.delta.y != 0);

  return ScaleY(ray, y, ray.size);
}

static constexpr GridLocation
NextBottomGridIntersection(const GridRay &ray,
                           const GridLocation &current) noexcept
{
  assert(ray.delta.y > 0);

  if (current.tile.y == ray.end.tile.y)
    return ray.end;

  assert(current.tile.y < ray.end.tile.y);
  assert(current.y < ray.end.y);

  int y = (current.tile.y + 1) * ray.tile_size.y;
  int x = XAtY(ray, y);
  assert(x >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_size,
                      IndexAtY(ray, y));
}

static constexpr GridLocation
NextTopGridIntersection(const GridRay &ray, GridLocation current) noexcept
{
  assert(ray.delta.y < 0);

  if (current.tile.y == ray.end.tile.y)
    return ray.end;

  assert(current.tile.y > ray.end.tile.y);
  assert(current.y > ray.end.y);

  unsigned tile_y = current.tile.y;
  if (current.remainder.y == 0) {
    --tile_y;
    if (tile_y == ray.end.tile.y)
      return ray.end;
  }

  int y = tile_y * ray.tile_size.y;
  int x = XAtY(ray, y);
  assert(x >= 0);

  return GridLocation(RasterLocation(x, y),
                      ray.tile_size,
                      IndexAtY(ray, y));
}

static constexpr GridLocation
NextVerticalGridIntersection(const GridRay &ray,
                             const GridLocation &current) noexcept
{
  assert(ray.start.index == 0);
  assert(ray.end.index == ray.size);

  if (ray.delta.y > 0)
    return NextBottomGridIntersection(ray, current);
  else if (ray.delta.y < 0)
    return NextTopGridIntersection(ray, current);
  else
    return ray.end;
}

static constexpr GridLocation
NextGridIntersection(const GridRay &ray,
                     const GridLocation &current) noexcept
{
  assert(ray.start.index == 0);
  assert(ray.end.index > 0);

  GridLocation h = NextHorizontalGridIntersection(ray, current);
  GridLocation v = NextVerticalGridIntersection(ray, current);

  return ManhattanDistance(h, ray.start) < ManhattanDistance(v, ray.start)
    ? h : v;
}

inline void
RasterTileCache::ScanTileLine(GridLocation start, GridLocation end,
                              TerrainHeight *buffer, unsigned size,
                              bool interpolate) const noexcept
{
  assert(end.index >= start.index);
  assert(end.index <= size);

  if (start.index == end.index)
    return;

  if (start.tile.x < end.tile.x) {
    assert(end.tile.x == start.tile.x + 1);
    assert(end.remainder.x == 0);

    --end.x;
  } else if (start.tile.x > end.tile.x) {
    assert(start.tile.x == end.tile.x + 1);
    assert(start.remainder.x == 0);

    --start.x;
    --start.tile.x;
  }

  if (start.tile.y < end.tile.y) {
    assert(end.tile.y == start.tile.y + 1);
    assert(end.remainder.y == 0);

    --end.y;
  } else if (start.tile.y > end.tile.y) {
    assert(start.tile.y == end.tile.y + 1);
    assert(start.remainder.y == 0);

    --start.y;
    --start.tile.y;
  }

  const RasterTile &tile = tiles.Get(start.tile.x, start.tile.y);
  if (tile.IsLoaded())
    tile.ScanLine(start, end,
                  buffer + start.index, end.index - start.index,
                  interpolate);
  else
    /* need range checking in the overview buffer because its size may
       be rounded down, and then the "fine" location may exceed its
       bounds */
    overview.ScanLineChecked(start >> RasterTraits::OVERVIEW_BITS,
                             end >> RasterTraits::OVERVIEW_BITS,
                             buffer + start.index, end.index - start.index,
                             interpolate);
}

void
RasterTileCache::ScanLine(const RasterLocation _start,
                          const RasterLocation _end,
                          TerrainHeight *buffer, unsigned size,
                          bool interpolate) const noexcept
{
  assert(_start.x < GetFineSize().x);
  assert(_start.y < GetFineSize().y);
  assert(_end.x < GetFineSize().x);
  assert(_end.y < GetFineSize().y);
  assert(size >= 2);

  const GridRay ray(GetFineTileSize(), _start, _end, size);
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
