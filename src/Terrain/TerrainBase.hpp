// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "CogRaster.hpp"
#include "RasterProjection.hpp"
#include "util/Serial.hpp"

#include <memory>

class FileCache;
class OperationEnvironment;
class Path;

/**
 * GeoTIFF elevation.  A map zip yields terrain.tif or a JP2
 * converted into a COG cache.
 */
class TerrainBase {
  CogRaster raster;
  RasterProjection projection;
  Serial serial;

public:
  TerrainBase() noexcept = default;

  TerrainBase(const TerrainBase &) = delete;
  TerrainBase &operator=(const TerrainBase &) = delete;

  const Serial &GetSerial() const noexcept {
    return serial;
  }

  [[gnu::pure]]
  TerrainHeight GetTerrainHeight(const GeoPoint location,
                                 unsigned quantisation = 1) const noexcept {
    return raster.GetHeight(location, quantisation);
  }

  void ScanTerrainLine(const GeoPoint start, const GeoPoint end,
                       TerrainHeight *buffer, unsigned size,
                       unsigned quantisation) const noexcept {
    raster.ScanLine(start, end, buffer, size, quantisation);
  }

  [[gnu::pure]]
  GeoPoint GetTerrainCenter() const noexcept {
    return GetBounds().GetCenter();
  }

  [[gnu::pure]]
  const GeoBounds &GetBounds() const noexcept {
    return raster.GetBounds();
  }

  [[gnu::pure]]
  bool IsDefined() const noexcept {
    return raster.IsOpen();
  }

  unsigned PreCacheTiles(const GeoBounds &bounds,
                         unsigned quantisation = 1) const noexcept {
    return raster.QueueInternalTiles(bounds, quantisation);
  }

  [[gnu::pure]]
  double PixelDistance(const GeoPoint &location,
                       unsigned pixels) const noexcept {
    return projection.CoarsePixelDistance(location, pixels);
  }

  bool UpdateTiles(const GeoPoint &location, double radius) noexcept;

  [[nodiscard]]
  bool HasPendingTiles() const noexcept {
    return raster.HasPendingInternalTiles();
  }

  /**
   * Open a local \c .tif, or a map zip (COG inside or JP2 converted).
   */
  static std::unique_ptr<TerrainBase> OpenTerrain(FileCache *cache,
                                                   Path path,
                                                   OperationEnvironment &operation);

  /**
   * Open terrain from profile settings.
   * TerrainFile GeoTIFF overrides elevation.  Else MapFile, or a zip
   * still stored in TerrainFile.
   */
  static std::unique_ptr<TerrainBase> OpenTerrain(FileCache *cache,
                                                   OperationEnvironment &operation);

private:
  void Load(OperationEnvironment &operation);

  static std::unique_ptr<TerrainBase>
  OpenCog(Path path, OperationEnvironment &operation);
};
