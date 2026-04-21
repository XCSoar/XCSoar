// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MVT.hpp"
#include "TileCoord.hpp"
#include "MapWindow/Overlay.hpp"
#include "Geo/GeoBounds.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

/**
 * A single parsed tile with its geo-bounds and MVT data.
 */
struct XCThermParsedTile {
  XCThermTileCoord coord;
  XCThermMVT::Tile data;
  GeoBounds bounds;

  XCThermParsedTile(XCThermTileCoord _coord,
                    XCThermMVT::Tile &&_data,
                    GeoBounds _bounds) noexcept
    : coord(_coord), data(std::move(_data)), bounds(_bounds) {}
};

/**
 * Composite MapOverlay that renders all XCTherm wave forecast tiles
 * in a single overlay slot. This avoids the need for multi-overlay
 * arrays in MapWindow (keeping the XCSoar upstream diff minimal).
 */
class XCThermCompositeOverlay final : public MapOverlay {
public:
  XCThermCompositeOverlay() noexcept;

  const char *GetLabel() const noexcept override;
  bool IsInside(GeoPoint p) const noexcept override;
  void Draw(Canvas &canvas,
            const WindowProjection &projection) noexcept override;

  /**
   * Add a parsed tile to this composite overlay.
   */
  void AddTile(XCThermTileCoord coord,
               XCThermMVT::Tile &&data) noexcept;

  /**
   * Remove all tiles.
   */
  void Clear() noexcept;

  /**
   * @return number of tiles currently held
   */
  unsigned GetTileCount() const noexcept;

private:
  static GeoBounds ComputeTileBounds(XCThermTileCoord coord) noexcept;

  static GeoPoint TilePointToGeo(XCThermTileCoord coord,
                                 uint32_t extent,
                                 int x, int y) noexcept;

  mutable std::mutex mutex;
  std::vector<XCThermParsedTile> tiles;
};
