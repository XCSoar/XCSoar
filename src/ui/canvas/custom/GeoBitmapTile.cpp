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

static uint32_t
LonToTileX(double lon, unsigned zoom) noexcept
{
  return uint32_t(std::floor((lon + 180.0) / 360.0 *
                             (uint32_t{1} << zoom)));
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

  const uint32_t tiles_per_axis = uint32_t{1} << zoom;
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
