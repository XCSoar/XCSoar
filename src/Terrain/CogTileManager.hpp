// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "RasterMap.hpp"

#include <string>
#include <map>
#include <memory>
#include <mutex>

class CurlGlobal;
class OperationEnvironment;
class CogLoader;

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
   * Format the tile ID as a filename (e.g., "N47E011.tif").
   */
  [[gnu::pure]]
  std::string ToFilename() const noexcept;
};

/**
 * Manages automatic downloading and caching of COG terrain tiles.
 *
 * Given a base URL template (e.g., "https://example.com/srtm/{tile}.tif"),
 * this class automatically fetches the appropriate tiles for the current
 * viewport and merges them into a single RasterMap.
 *
 * Tile naming follows SRTM conventions:
 * - N47E011.tif for latitude 47°N, longitude 11°E
 * - S12W034.tif for latitude 12°S, longitude 34°W
 */
class CogTileManager {
public:
  /**
   * Supported tile server providers with known URL patterns.
   */
  enum class Provider {
    OPENTOPOGRAPHY_SRTM_GL1,
    OPENTOPOGRAPHY_SRTM_GL3,
    CUSTOM,
  };

private:
  std::string base_url;
  Provider provider;

  CurlGlobal *curl = nullptr;

  mutable std::mutex tiles_mutex;
  std::map<CogTileId, std::unique_ptr<CogLoader>> loaded_tiles;

  GeoPoint last_center = GeoPoint::Invalid();
  double last_radius = 0;

public:
  CogTileManager() noexcept = default;

  /**
   * Initialize with a provider preset.
   */
  void SetProvider(Provider p, CurlGlobal &curl) noexcept;

  /**
   * Initialize with a custom base URL.
   * Use {N}, {S}, {E}, {W}, {LAT}, {LON} placeholders in the URL.
   * Example: "https://example.com/srtm/{N}{LAT}{E}{LON}.tif"
   */
  void SetCustomUrl(const char *url, CurlGlobal &curl) noexcept;

  /**
   * Get the full URL for a specific tile.
   */
  [[gnu::pure]]
  std::string GetTileUrl(const CogTileId &tile) const noexcept;

  /**
   * Update tiles for the given viewport.
   * Downloads any missing tiles that intersect the viewport.
   *
   * @param center center of the viewport
   * @param radius radius in meters
   * @param map RasterMap to update with new terrain data
   * @param operation for progress reporting
   * @return true if more updates are pending
   */
  bool UpdateTiles(const GeoPoint &center, double radius,
                   RasterMap &map,
                   OperationEnvironment &operation) noexcept;

  /**
   * Check if a tile is already loaded.
   */
  [[gnu::pure]]
  bool IsTileLoaded(const CogTileId &tile) const noexcept;

  /**
   * Get the number of loaded tiles.
   */
  [[gnu::pure]]
  size_t GetLoadedTileCount() const noexcept;

  /**
   * Clear all loaded tiles.
   */
  void Clear() noexcept;

  /**
   * Get terrain height at a location from loaded tiles.
   * Returns TerrainHeight::Invalid() if no tile covers the location.
   *
   * @param location geographic location to query
   * @param quantisation desired pixel quantisation for pyramid level selection
   */
  [[gnu::pure]]
  TerrainHeight GetHeight(const GeoPoint &location,
                          unsigned quantisation = 1) const noexcept;

  /**
   * Get the combined bounds of all loaded tiles.
   * Returns invalid bounds if no tiles are loaded.
   */
  [[gnu::pure]]
  GeoBounds GetBounds() const noexcept;

  /**
   * Check if any tiles are loaded.
   */
  [[gnu::pure]]
  bool HasLoadedTiles() const noexcept {
    const std::lock_guard lock{tiles_mutex};
    return !loaded_tiles.empty();
  }

  /**
   * Pre-cache all internal tiles needed to cover a bounding box.
   * This avoids per-pixel HTTP requests during height matrix fill.
   *
   * @param bounds geographic bounds to cover
   * @param quantisation desired pixel quantisation for level selection
   * @return number of tiles loaded
   */
  unsigned PreCacheTiles(const GeoBounds &bounds,
                         unsigned quantisation = 1) const noexcept;

  /**
   * Get the base URL for the provider.
   */
  static const char *GetProviderUrl(Provider p) noexcept;

  /**
   * Parse a provider name from string.
   */
  static Provider ParseProvider(const char *name) noexcept;
};
