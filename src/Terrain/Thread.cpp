// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Thread.hpp"
#include "RasterTerrain.hpp"
#include "Projection/WindowProjection.hpp"
#include "thread/Util.hpp"

TerrainThread::TerrainThread(RasterTerrain &_terrain,
                             std::function<void()> &&_callback)
  :StandbyThread("Terrain"), terrain(_terrain),
   callback(std::move(_callback)) {}

void
TerrainThread::Trigger(const WindowProjection &projection)
{
  assert(projection.IsValid());

  const std::lock_guard lock{mutex};

  GeoPoint center = projection.GetGeoScreenCenter();
  auto radius = projection.GetScreenWidthMeters() / 2;
  if (last_center.IsValid() && last_radius >= radius &&
      last_center.DistanceS(center) < 1000)
    return;

  next_center = center;
  next_radius = radius;
  const double scale = projection.GetScale();
  next_pixel_size = scale > 0 ? 1. / scale : 0;
  StandbyThread::Trigger();
}

void
TerrainThread::Tick() noexcept
{
  SetIdlePriority(); // TODO: call only once

  bool again = true;
  while (next_center.IsValid() && again && !IsStopped()) {
    const GeoPoint center = next_center;
    const auto radius = next_radius;
    const auto pixel_size = next_pixel_size;

    {
      const ScopeUnlock unlock(mutex);
      again = terrain.UpdateTiles(center, radius, pixel_size);
    }

    last_center = center;
    last_radius = radius;
  }

  /* notify the client */
  if (callback) {
    const ScopeUnlock unlock(mutex);
    callback();
  }
}
