// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "io/Sqlite.hpp"
#include "Geo/GeoBounds.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"
#include "util/StaticString.hxx"

#include <compare>
#include <cstdint>
#include <mutex>

struct MbTilesMetadata {
  StaticString<32> format;
  GeoBounds bounds = GeoBounds::Invalid();
  unsigned min_zoom = 0;
  unsigned max_zoom = 0;
};

/**
 * One MBTiles tile identifier in TMS row/column space.
 */
struct TileKey {
  unsigned zoom;
  unsigned column;
  unsigned row;

  static TileKey FromGeoPoint(GeoPoint point, unsigned zoom) noexcept;

  GeoPoint GetNorthWest() const noexcept;
  GeoPoint GetNorthEast() const noexcept;
  GeoPoint GetSouthWest() const noexcept;
  GeoPoint GetSouthEast() const noexcept;

  constexpr auto operator<=>(const TileKey &) const noexcept = default;
};

struct Rgba8 {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 0;
};

class MbTilesDatabase {
  mutable std::mutex mutex;
  SqliteDatabase db;
  MbTilesMetadata metadata;

  [[gnu::pure]]
  bool HasTileUnlocked(TileKey key) const;

public:
  explicit MbTilesDatabase(Path path);
  MbTilesDatabase(MbTilesDatabase &&other) noexcept = delete;
  MbTilesDatabase &operator=(MbTilesDatabase &&other) noexcept = delete;
  ~MbTilesDatabase() noexcept = default;

  MbTilesDatabase(const MbTilesDatabase &) = delete;
  MbTilesDatabase &operator=(const MbTilesDatabase &) = delete;

  const MbTilesMetadata &GetMetadata() const noexcept {
    return metadata;
  }

  [[gnu::pure]]
  bool HasTile(TileKey key) const;

  Bitmap LoadTile(TileKey key) const;

  /**
   * Sample one RGBA pixel at @p point from the tile at maximum zoom.
   */
  bool SampleRgbaAtGeo(GeoPoint p, Rgba8 &pixel) const noexcept;
};
