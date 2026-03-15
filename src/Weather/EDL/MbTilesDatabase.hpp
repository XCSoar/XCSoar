// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Sqlite.hpp"
#include "Geo/GeoBounds.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "util/StaticString.hxx"

#include <compare>
#include <cstddef>
#include <vector>

namespace EDL {

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

  /**
   * Convert a geographic position to the MBTiles tile covering it.
   */
  static TileKey FromGeoPoint(GeoPoint point, unsigned zoom) noexcept;

  /**
   * Return the north-west corner of this tile.
   */
  GeoPoint GetNorthWest() const noexcept;

  /**
   * Return the north-east corner of this tile.
   */
  GeoPoint GetNorthEast() const noexcept;

  /**
   * Return the south-west corner of this tile.
   */
  GeoPoint GetSouthWest() const noexcept;

  /**
   * Return the south-east corner of this tile.
   */
  GeoPoint GetSouthEast() const noexcept;

  constexpr auto operator<=>(const TileKey &) const noexcept = default;
};

/**
 * Read tile metadata and tile bitmaps from an MBTiles database.
 */
class MbTilesDatabase {
  SqliteDatabase db;
  MbTilesMetadata metadata;

public:
  explicit MbTilesDatabase(Path path);
  MbTilesDatabase(MbTilesDatabase &&other) noexcept = default;
  MbTilesDatabase &operator=(MbTilesDatabase &&other) noexcept = default;
  ~MbTilesDatabase() noexcept = default;

  MbTilesDatabase(const MbTilesDatabase &) = delete;
  MbTilesDatabase &operator=(const MbTilesDatabase &) = delete;

  const MbTilesMetadata &GetMetadata() const noexcept {
    return metadata;
  }

  /**
   * Check whether the specified tile exists in the database.
   */
  [[gnu::pure]]
  bool
  HasTile(TileKey key) const;

  /**
   * Load and decode one tile bitmap from the database.
   */
  Bitmap
  LoadTile(TileKey key) const;
};

} // namespace EDL
