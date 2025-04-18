// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class TopographyStore;
class RasterTerrain;
class Waypoints;
class Airspaces;

/**
 * This singleton manages all data loaded from data files (waypoints,
 * airspaces, etc.) and associated objects (caches, loaders) but not
 * user- or interface-specific runtime state (e.g. no airspace
 * warnings).
 */
struct DataComponents {
  const std::unique_ptr<Airspaces> airspaces;
  const std::unique_ptr<Waypoints> waypoints;

  std::unique_ptr<TopographyStore> topography;

  std::unique_ptr<RasterTerrain> terrain;

  DataComponents() noexcept;
  ~DataComponents() noexcept;

  DataComponents(const DataComponents &) = delete;
  DataComponents &operator=(const DataComponents &) = delete;
};
