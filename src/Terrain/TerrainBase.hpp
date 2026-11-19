// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RasterMap.hpp"
#include "Geo/GeoPoint.hpp"
#include "thread/Guard.hpp"

#include <memory>

class FileCache;
class OperationEnvironment;

/**
 * Abstract base class for terrain implementations.
 *
 * This provides a common interface for different terrain backends:
 * - RasterTerrain: Loads from .xcm (ZIP) archives with JP2 terrain
 * - CogTerrain: Loads from COG files (local or HTTP)
 */
class TerrainBase : public Guard<RasterMap> {
protected:
  RasterMap map;

public:
  TerrainBase() noexcept
    :Guard<RasterMap>(map) {}

  virtual ~TerrainBase() noexcept = default;

  TerrainBase(const TerrainBase &) = delete;
  TerrainBase &operator=(const TerrainBase &) = delete;

  const Serial &GetSerial() const noexcept {
    return map.GetSerial();
  }

  /**
   * Get terrain height at a location.
   *
   * @param location geographic location
   * @param quantisation pixel quantisation hint for pyramid level selection
   *        (1=full resolution, higher values may select lower-resolution data)
   */
  [[gnu::pure]]
  virtual TerrainHeight GetTerrainHeight(const GeoPoint location,
                                         unsigned quantisation = 1) const noexcept {
    (void)quantisation;
    Lease lease(*this);
    return lease->GetHeight(location);
  }

  [[gnu::pure]]
  virtual GeoPoint GetTerrainCenter() const noexcept {
    return map.GetMapCenter();
  }

  [[gnu::pure]]
  virtual GeoBounds GetBounds() const noexcept {
    return map.GetBounds();
  }

  [[gnu::pure]]
  virtual bool IsDefined() const noexcept {
    return map.IsDefined();
  }

  /**
   * Check if this terrain is in auto-tile mode (tiles download on demand).
   * Only CogTerrain can be in auto-tile mode; XcmTerrain always returns false.
   */
  [[gnu::pure]]
  virtual bool IsAutoTileMode() const noexcept {
    return false;
  }

  /**
   * Get a pointer to the underlying RasterMap.
   * This is used by components that need direct access to the map
   * (e.g., route planner).
   */
  const RasterMap *GetMap() const noexcept {
    return &map;
  }

  /**
   * @return true if the method shall be called again
   */
  virtual bool UpdateTiles(const GeoPoint &location, double radius) noexcept = 0;

  /**
   * Open terrain from profile settings.
   * Checks TerrainFile first (for COG), then MapFile (for .xcm).
   *
   * @param cache file cache (may be nullptr)
   * @param operation operation environment for progress reporting
   * @return terrain instance, or nullptr if no terrain configured
   */
  static std::unique_ptr<TerrainBase> OpenTerrain(FileCache *cache,
                                                   OperationEnvironment &operation);
};
