// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/StandbyThread.hpp"
#include "Projection/MapLod.hpp"
#include "Geo/GeoPoint.hpp"

#include <functional>

class RasterTerrain;
class WindowProjection;

/**
 * A thread that loads terrain tiles asynchronously.
 */
class TerrainThread final : private StandbyThread {
  RasterTerrain &terrain;

  const std::function<void()> callback;

  GeoPoint last_center = GeoPoint::Invalid();
  double last_radius;
  unsigned last_lod = MAP_LOD_COUNT;

  GeoPoint next_center;
  double next_radius;
  double next_pixel_size = 0;
  unsigned next_lod = 0;

public:
  TerrainThread(RasterTerrain &_terrain, std::function<void()> &&_callback);

  using StandbyThread::LockStop;

  void Trigger(const WindowProjection &projection);

private:
  /* virtual methods from class StandbyThread*/
  void Tick() noexcept override;
};
