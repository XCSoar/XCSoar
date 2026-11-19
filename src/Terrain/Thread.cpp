// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Thread.hpp"
#include "TerrainBase.hpp"
#include "Projection/WindowProjection.hpp"
#include "thread/Util.hpp"

TerrainThread::TerrainThread(TerrainBase &_terrain,
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

  bool should_trigger = false;

  if (!last_center.IsValid()) {
    should_trigger = true;
  } else if (last_radius < radius) {
    should_trigger = true;
  } else if (last_center.DistanceS(center) >= 100) {
    should_trigger = true;
  } else if (terrain.HasPendingTiles()) {
    should_trigger = true;
  }

  if (!should_trigger)
    return;

  next_center = center;
  next_radius = radius;
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

    {
      const ScopeUnlock unlock(mutex);
      again = terrain.UpdateTiles(center, radius);
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
