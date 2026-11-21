// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Height.hpp"
#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "RasterLocation.hpp"
#include "system/Path.hpp"

#include <memory>

/**
 * One pyramid level in a Cloud-Optimized GeoTIFF.
 */
struct CogLevel {
  unsigned width = 0;
  unsigned height = 0;
  unsigned tile_width = 0;
  unsigned tile_height = 0;
  unsigned tiles_x = 0;
  unsigned tiles_y = 0;
  unsigned directory = 0;

  /** 1, 2, 4, … relative to full resolution. */
  unsigned scale = 1;

  int16_t nodata = -32768;
};

/**
 * One GeoTIFF: open, decode tiles, RAM pyramid cache, sample.
 * GetHeight / ScanLine are cache-only (draw thread).
 * Queue / ProcessOne decode on the terrain thread.
 */
class CogRaster {
  struct Impl;
  std::unique_ptr<Impl> impl;

public:
  CogRaster() noexcept;
  ~CogRaster() noexcept;

  CogRaster(const CogRaster &) = delete;
  CogRaster &operator=(const CogRaster &) = delete;

  [[gnu::pure]]
  static bool IsCogFile(const char *path) noexcept;

  bool Open(Path path);
  void Close() noexcept;

  [[gnu::pure]]
  bool IsOpen() const noexcept;

  [[gnu::pure]]
  const GeoBounds &GetBounds() const noexcept;

  [[gnu::pure]]
  UnsignedPoint2D GetSize() const noexcept;

  [[gnu::pure]]
  unsigned GetLevelCount() const noexcept;

  [[gnu::pure]]
  const CogLevel &GetLevel(unsigned level) const noexcept;

  [[gnu::pure]]
  UnsignedPoint2D GetTileSize(unsigned level) const noexcept;

  [[gnu::pure]]
  TerrainHeight GetHeight(const GeoPoint &location,
                          unsigned quantisation = 1) const noexcept;

  void ScanLine(const GeoPoint start, const GeoPoint end,
                TerrainHeight *buffer, unsigned size,
                unsigned quantisation) const noexcept;

  unsigned QueueInternalTiles(const GeoBounds &bounds,
                              unsigned quantisation = 1) const noexcept;

  bool ProcessScanTile() noexcept;

  bool ProcessOneInternalTile() noexcept;

  [[gnu::pure]]
  bool HasPendingInternalTiles() const noexcept;
};
