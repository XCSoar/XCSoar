// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TerrainBase.hpp"
#include "CogLoader.hpp"

#include <string>

class Path;
class CurlGlobal;

/**
 * Terrain manager for Cloud-Optimized GeoTIFF (COG) files.
 *
 * Unlike RasterTerrain which loads from .xcm archives, this class
 * loads terrain directly from COG files, either local or via HTTP.
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

  std::string source_path;

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

  bool UpdateTiles(const GeoPoint &location, double radius) noexcept override;

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
