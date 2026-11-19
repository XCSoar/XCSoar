// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TerrainBase.hpp"
#include "CogLoader.hpp"
#include "CogTileManager.hpp"

#include <string>

class Path;
class CurlGlobal;

/**
 * Terrain manager for Cloud-Optimized GeoTIFF (COG) files.
 *
 * Unlike RasterTerrain which loads from .xcm archives, this class
 * loads terrain directly from COG files, either local or via HTTP.
 *
 * Supports two modes:
 * 1. Single-file mode: Load a specific COG file (local or HTTP)
 * 2. Auto-tile mode: Automatically download tiles based on location
 *    using predefined providers (e.g., OpenTopography SRTM)
 *
 * The same CogTerrain instance is compatible with the existing
 * terrain rendering infrastructure through its RasterMap.
 */
class CogTerrain final : public TerrainBase {
public:
  friend class RoutePlannerGlue;
  friend class ProtectedTaskManager;
  friend class WaypointVisitorMap;

private:
  CogLoader loader;
  CogTileManager tile_manager;

  std::string source_path;

  bool auto_tile_mode = false;

#ifdef HAVE_HTTP
  CurlGlobal *curl = nullptr;
#endif

public:
  CogTerrain() noexcept = default;

  /**
   * Check if a path/URL is a COG terrain file (by extension).
   */
  [[gnu::pure]]
  static bool IsCogTerrain(const char *path) noexcept {
    return CogLoader::IsCogFile(path);
  }

  /**
   * Check if a path/URL is an HTTP URL.
   */
  [[gnu::pure]]
  static bool IsHttpUrl(const char *path) noexcept {
    return CogLoader::IsHttpUrl(path);
  }

  /**
   * Open terrain from a local COG file.
   *
   * Throws on error.
   *
   * @param cache file cache for terrain data (may be nullptr)
   * @param path local filesystem path to the COG file
   * @param operation operation environment for progress
   * @return terrain instance, or nullptr if path is null
   */
  static std::unique_ptr<CogTerrain> OpenTerrain(FileCache *cache,
                                                  Path path,
                                                  OperationEnvironment &operation);

  /**
   * Open terrain from an HTTP URL.
   *
   * Throws on error.
   *
   * @param curl curl instance for HTTP requests
   * @param url HTTP/HTTPS URL of the COG file
   * @param operation operation environment for progress
   * @return terrain instance
   */
  static std::unique_ptr<CogTerrain> OpenTerrainHttp(CurlGlobal &curl,
                                                      const char *url,
                                                      OperationEnvironment &operation);

  /**
   * Open terrain from either a local path or HTTP URL.
   * Automatically detects which based on the path prefix.
   *
   * Throws on error.
   *
   * @param cache file cache (used only for local files, may be nullptr)
   * @param curl curl instance (used only for HTTP URLs)
   * @param path_or_url local path or HTTP URL
   * @param operation operation environment for progress
   * @return terrain instance
   */
  static std::unique_ptr<CogTerrain> OpenTerrain(FileCache *cache,
                                                  CurlGlobal *curl,
                                                  const char *path_or_url,
                                                  OperationEnvironment &operation);

  /**
   * Open terrain in auto-tile mode using a predefined provider.
   * Tiles will be downloaded automatically as needed based on location.
   *
   * @param curl curl instance for HTTP requests
   * @param provider provider name (e.g., "srtm_gl1", "srtm_gl3")
   * @return terrain instance (not yet loaded - call UpdateTiles)
   */
  static std::unique_ptr<CogTerrain> OpenAutoTile(CurlGlobal &curl,
                                                   const char *provider);

  /**
   * Check if a provider name indicates auto-tile mode.
   */
  [[gnu::pure]]
  static bool IsAutoTileProvider(const char *name) noexcept;

  bool UpdateTiles(const GeoPoint &location, double radius) noexcept override;

  /**
   * Get terrain height, using tile manager in auto-tile mode.
   */
  [[gnu::pure]]
  TerrainHeight GetTerrainHeight(const GeoPoint location) const noexcept override;

  /**
   * Check if this terrain is in auto-tile mode.
   */
  [[gnu::pure]]
  bool IsAutoTileMode() const noexcept override {
    return auto_tile_mode;
  }

  /**
   * Check if terrain data is available (tiles loaded in auto-tile mode).
   */
  [[gnu::pure]]
  bool IsDefined() const noexcept override {
    if (auto_tile_mode)
      return tile_manager.HasLoadedTiles();
    return map.IsDefined();
  }

  /**
   * Get terrain center, using tile manager bounds in auto-tile mode.
   */
  [[gnu::pure]]
  GeoPoint GetTerrainCenter() const noexcept override {
    if (auto_tile_mode) {
      const auto bounds = tile_manager.GetBounds();
      if (bounds.IsValid())
        return bounds.GetCenter();
      return GeoPoint::Invalid();
    }
    return map.GetMapCenter();
  }

  /**
   * Get terrain bounds, using tile manager in auto-tile mode.
   */
  [[gnu::pure]]
  GeoBounds GetBounds() const noexcept override {
    if (auto_tile_mode)
      return tile_manager.GetBounds();
    return map.GetBounds();
  }

  /**
   * Get the source path or URL.
   */
  [[gnu::pure]]
  const std::string &GetSourcePath() const noexcept {
    return source_path;
  }

  /**
   * @return true if terrain is loaded from HTTP
   */
  [[gnu::pure]]
  bool IsHttp() const noexcept {
    return loader.IsHttp();
  }

private:
  void Load(OperationEnvironment &operation);
};
