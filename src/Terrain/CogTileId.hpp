// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Geo/GeoBounds.hpp"

#include <cmath>
#include <string>

/**
 * Tile identifier for SRTM-style 1-degree tiles.
 * Format: N47E011 for latitude 47°N, longitude 11°E
 */
struct CogTileId {
  int lat;
  int lon;

  constexpr bool operator<(const CogTileId &other) const noexcept {
    if (lat != other.lat)
      return lat < other.lat;
    return lon < other.lon;
  }

  constexpr bool operator==(const CogTileId &other) const noexcept {
    return lat == other.lat && lon == other.lon;
  }

  /**
   * Get the tile ID for a given location.
   */
  static constexpr CogTileId FromLocation(const GeoPoint &location) noexcept {
    return {
      static_cast<int>(std::floor(location.latitude.Degrees())),
      static_cast<int>(std::floor(location.longitude.Degrees()))
    };
  }

  /**
   * Get the bounds covered by this tile.
   */
  [[gnu::pure]]
  GeoBounds GetBounds() const noexcept {
    return GeoBounds{
      GeoPoint{Angle::Degrees(lon), Angle::Degrees(lat + 1)},
      GeoPoint{Angle::Degrees(lon + 1), Angle::Degrees(lat)}
    };
  }

  /**
   * Format the tile ID as a filename (e.g., "N47E011.tif").
   */
  [[gnu::pure]]
  std::string ToFilename() const noexcept;
};
