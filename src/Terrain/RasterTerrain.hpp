// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RasterMap.hpp"
#include "Geo/GeoPoint.hpp"
#include "thread/Guard.hpp"
#include "io/ZipArchive.hpp"

#include <memory>

class Path;
class FileCache;
class OperationEnvironment;

/**
 * Class to manage raster terrain database, potentially with caching
 * or demand-loading.
 */
class RasterTerrain : public Guard<RasterMap> {
public:
  friend class RoutePlannerGlue; // for route planning
  friend class ProtectedTaskManager; // for intersection
  friend class WaypointVisitorMap; // for intersection rendering

private:
  ZipArchive archive;

  RasterMap map;

public:
  /**
   * Constructor.  Returns uninitialised object.
   */
  explicit RasterTerrain(ZipArchive &&_archive) noexcept
    :Guard<RasterMap>(map), archive(std::move(_archive)) {}

  const Serial &GetSerial() const noexcept {
    return map.GetSerial();
  }

  /**
   * Throws on error.
   */
  static std::unique_ptr<RasterTerrain> OpenTerrain(FileCache *cache,
                                                    Path path,
                                                    OperationEnvironment &operation);

  /**
   * Load the terrain.  Determines the file to load from profile settings.
   */
  static std::unique_ptr<RasterTerrain> OpenTerrain(FileCache *cache,
                                                    OperationEnvironment &operation);

  [[gnu::pure]]
  TerrainHeight GetTerrainHeight(const GeoPoint location) const noexcept {
    Lease lease(*this);
    return lease->GetHeight(location);
  }

  GeoPoint GetTerrainCenter() const noexcept {
    return map.GetMapCenter();
  }

  /**
   * Load fine tiles around #location.
   *
   * @param pixel_size_m metres covered by one screen pixel.  When
   * this is at least one step-grid sample, only the tiles next to
   * #location are loaded; the rest of the view uses the step grid.
   * Zero keeps the full #radius.
   *
   * @return true if the method shall be called again
   */
  bool UpdateTiles(const GeoPoint &location, double radius,
                   double pixel_size_m = 0) noexcept;

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
