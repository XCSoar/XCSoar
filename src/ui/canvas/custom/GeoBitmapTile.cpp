// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * The slippy map tile geometry, kept apart from GeoBitmap.cpp so that
 * it links without a canvas: the tile grid is useful to code that
 * plans downloads long before there is anything to draw, and that
 * code should not have to pull in OpenGL to ask where a tile is.
 */

#include "GeoBitmap.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/Quadrilateral.hpp"

#include <algorithm>
#include <cmath>

using namespace GeoBitmap;

/** The number of tiles along each axis, for a zoom we can shift by. */
static constexpr uint32_t
TilesPerAxis(unsigned zoom) noexcept
{
  /* the shift is undefined once zoom reaches the width of the type,
     and TileData carries a uint16_t that nothing else validates */
  return uint32_t{1} << std::min(zoom, unsigned(GeoBitmap::MAX_TILE_ZOOM));
}

static uint32_t
LonToTileX(double lon, unsigned zoom) noexcept
{
  const uint32_t tiles_per_axis = TilesPerAxis(zoom);
  const double scaled = (lon + 180.0) / 360.0 * tiles_per_axis;
  const double x = std::floor(scaled);

  if (x >= 0 && x < double(tiles_per_axis))
    return uint32_t(x);

  if (x == double(tiles_per_axis))
    /* the antimeridian, one index past the last column.  It is the
       eastern edge of that column, and that is the tile a flight
       reaching it is in; wrapping to the first column would put the
       tile on the far side of the map, because
       GeoQuadrilateral::GetBounds() does not wrap.

       Tested against the index rather than the exact longitude: a
       degree value that has been through Angle's radians and back is
       not exactly 180 any more, and the boundary has to hold for what
       actually arrives. */
    return tiles_per_axis - 1;

  /* anywhere else outside the grid is a longitude that has run past
     the world, which the projection simply wraps.  The cast of an
     out-of-range double to uint32_t would be undefined, so wrap
     first. */
  const double wrapped = std::fmod(x, double(tiles_per_axis));
  return uint32_t(wrapped < 0 ? wrapped + tiles_per_axis : wrapped);
}

static uint32_t
LatToTileY(double lat, unsigned zoom) noexcept
{
  /* the grid is only defined between the Web Mercator cut-offs, where
     the projection runs to infinity; beyond them tan() explodes and
     the cast below would be undefined.  A wild fix must yield an edge
     tile, not chaos. */
  constexpr double MERCATOR_LIMIT = 85.05112878;
  const double latitude_radians =
    std::clamp(lat, -MERCATOR_LIMIT, MERCATOR_LIMIT) * M_PI / 180.0;

  const uint32_t tiles_per_axis = TilesPerAxis(zoom);
  const double y = std::floor(
    (1.0 - std::asinh(std::tan(latitude_radians)) / M_PI) / 2.0 *
    tiles_per_axis);

  return uint32_t(std::clamp(y, 0.0, double(tiles_per_axis - 1)));
}

static double
TileXToLon(uint32_t x, unsigned zoom) noexcept
{
  return x / double(uint32_t{1} << zoom) * 360.0 - 180.0;
}

static double
TileYToLat(uint32_t y, unsigned zoom) noexcept
{
  const double n = M_PI - 2.0 * M_PI * y /
    double(uint32_t{1} << zoom);
  return 180.0 / M_PI * std::atan(0.5 * (std::exp(n) - std::exp(-n)));
}

TileData
GeoBitmap::GetTile(const GeoBounds &bounds, uint16_t zoom) noexcept
{
  return {
    zoom,
    LonToTileX(bounds.GetCenter().longitude.Degrees(), zoom),
    LatToTileY(bounds.GetCenter().latitude.Degrees(), zoom),
  };
}

GeoQuadrilateral
GeoBitmap::GetGeoQuadrilateral(const TileData &tile) noexcept
{
  GeoQuadrilateral bounds;

  bounds.top_left.longitude = Angle::Degrees(TileXToLon(tile.x, tile.zoom));
  bounds.top_left.latitude = Angle::Degrees(TileYToLat(tile.y, tile.zoom));
  bounds.bottom_left.longitude = bounds.top_left.longitude;
  bounds.bottom_left.latitude = Angle::Degrees(TileYToLat(tile.y + 1, tile.zoom));

  bounds.top_right.longitude = Angle::Degrees(TileXToLon(tile.x + 1, tile.zoom));
  bounds.top_right.latitude = bounds.top_left.latitude;
  bounds.bottom_right.longitude = bounds.top_right.longitude;
  bounds.bottom_right.latitude = bounds.bottom_left.latitude;

  return bounds;
}

GeoBounds
GeoBitmap::GetBounds(const TileData &tile) noexcept
{
  return GetGeoQuadrilateral(tile).GetBounds();
}
