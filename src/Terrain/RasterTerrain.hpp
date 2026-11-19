// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TerrainBase.hpp"
#include "io/ZipArchive.hpp"

class Path;

/**
 * Class to manage raster terrain database from .xcm (ZIP) archives,
 * potentially with caching or demand-loading.
 */
class XcmTerrain final : public TerrainBase {
public:
  friend class RoutePlannerGlue;
  friend class ProtectedTaskManager;
  friend class WaypointVisitorMap;

private:
  ZipArchive archive;

public:
  explicit XcmTerrain(ZipArchive &&_archive) noexcept
    :archive(std::move(_archive)) {}

  /**
   * Throws on error.
   */
  static std::unique_ptr<XcmTerrain> OpenTerrain(FileCache *cache,
                                                    Path path,
                                                    OperationEnvironment &operation);

  /**
   * Load the terrain from .xcm archive. Determines the file to load
   * from profile settings (ProfileKeys::MapFile).
   *
   * Note: For COG terrain support, use TerrainBase::OpenTerrain()
   * which checks TerrainFile first, then falls back to MapFile.
   */
  static std::unique_ptr<XcmTerrain> OpenTerrain(FileCache *cache,
                                                    OperationEnvironment &operation);

  bool UpdateTiles(const GeoPoint &location, double radius) noexcept override;

private:
  /**
   * Throws on error.
   */
  bool LoadCache(FileCache &cache, Path path);

  /**
   * Throws on error.
   */
  bool LoadCache(FileCache *cache, Path path);

  /**
   * Throws on error.
   */
  void SaveCache(FileCache &cache, Path path) const;

  /**
   * Throws on error.
   */
  void Load(Path path, FileCache *cache,
            OperationEnvironment &operation);
};
