// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/StandbyThread.hpp"
#include "Geo/GeoPoint.hpp"

#include <functional>

class RasterTerrain;
class WindowProjection;

/**
 * A thread that loads topography files asynchronously.
 */
class TerrainThread final : private StandbyThread {
  RasterTerrain &terrain;

  const std::function<void()> callback;

  GeoPoint last_center = GeoPoint::Invalid();
  double last_radius;

  GeoPoint next_center;
  double next_radius;

public:
  TerrainThread(RasterTerrain &_terrain, std::function<void()> &&_callback);

  using StandbyThread::LockStop;

  void Trigger(const WindowProjection &projection);

private:
  /* virtual methods from class StandbyThread*/
  void Tick() noexcept override;
};
