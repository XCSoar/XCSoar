// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "CogTileId.hpp"
#include "Geo/GeoPoint.hpp"
#include "RasterMap.hpp"

#include <string>
#include <map>
#include <set>
#include <memory>
#include <mutex>

class CurlGlobal;
class OperationEnvironment;
class CogLoader;

/**
 * Manages automatic downloading and caching of COG terrain tiles.
 *
 * Architecture:
 * - UpdateTiles() is non-blocking: queues needed tiles, returns immediately
 * - ProcessOneRequest() does actual I/O: call from background thread
 * - GetHeight() uses only loaded tiles: never blocks on I/O
 * - Disk cache checked before network requests
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
  std::string cache_path;
  Provider provider = Provider::OPENTOPOGRAPHY_SRTM_GL1;

  CurlGlobal *curl = nullptr;

  mutable std::mutex tiles_mutex;
  std::map<CogTileId, std::unique_ptr<CogLoader>> loaded_tiles;
  std::set<CogTileId> pending_tiles;
  std::set<CogTileId> failed_tiles;

  GeoPoint last_center{GeoPoint::Invalid()};
  double last_radius = 0;

  /**
   * Get the cache file path for a tile.
   */
  [[gnu::pure]]
  std::string GetCachePath(const CogTileId &tile) const noexcept;

public:
  CogTileManager() noexcept = default;

  /**
   * Set the cache directory path for persistent tile storage.
   */
  void SetCachePath(const char *path) noexcept {
    cache_path = path ? path : "";
  }

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
   * Queue tiles needed for the given viewport.
   * This is non-blocking - just identifies needed tiles and queues them.
   *
   * @param center center of the viewport
   * @param radius radius in meters
   * @return true if there are pending tiles to load
   */
  bool UpdateTiles(const GeoPoint &center, double radius) noexcept;

  /**
   * Process one pending tile request (blocking I/O).
   * Call this from a background thread.
   *
   * @return true if a tile was processed, false if queue is empty
   */
  bool ProcessOneRequest() noexcept;

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
   * Check if there are pending tile downloads.
   */
  [[gnu::pure]]
  bool HasPendingTiles() const noexcept {
    const std::lock_guard lock{tiles_mutex};
    return !pending_tiles.empty();
  }

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
   * Queue internal tiles needed to cover a bounding box for background loading.
   *
   * @param bounds geographic bounds to cover
   * @param quantisation desired pixel quantisation for level selection
   * @return number of tiles queued
   */
  unsigned PreCacheTiles(const GeoBounds &bounds,
                         unsigned quantisation = 1) const noexcept;

  /**
   * Process one pending internal tile (blocking I/O).
   * Call from background thread only.
   *
   * @return true if a tile was processed
   */
  bool ProcessInternalTiles() noexcept;

  /**
   * Check if any tiles have pending internal tiles to load.
   */
  [[gnu::pure]]
  bool HasPendingInternalTiles() const noexcept;

  /**
   * Get the base URL for the provider.
   */
  static const char *GetProviderUrl(Provider p) noexcept;

  /**
   * Parse a provider name from string.
   */
  static Provider ParseProvider(const char *name) noexcept;
};
