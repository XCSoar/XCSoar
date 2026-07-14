// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

class GeoBounds;
struct GeoQuadrilateral;
class MapWindowProjection;

namespace GeoBitmap {

inline constexpr unsigned MAX_TILE_ZOOM = 20;

struct TileData {
  uint16_t zoom = 0;
  uint32_t x = 0;
  uint32_t y = 0;

  constexpr bool IsValid() const noexcept {
    return zoom > 0;
  }
};

TileData
GetTile(const GeoBounds &bounds, uint16_t zoom) noexcept;

TileData
GetTile(const MapWindowProjection &projection,
        uint16_t zoom_min = 1,
        uint16_t zoom_max = MAX_TILE_ZOOM) noexcept;

GeoBounds
GetBounds(const TileData &data) noexcept;

GeoQuadrilateral
GetGeoQuadrilateral(const TileData &data) noexcept;

} // namespace GeoBitmap
