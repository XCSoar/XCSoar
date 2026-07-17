// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Thread.hpp"
#include "RasterTerrain.hpp"
#include "Projection/WindowProjection.hpp"
#include "thread/Util.hpp"

#include <algorithm>

/**
 * Above this #WindowProjection::GetMapScale(), fine terrain tiles are
 * not worth loading: the overview already covers the screen, and a
 * half-screen radius would pull in thousands of kilometres of tiles.
 */
static constexpr double FINE_TILE_MAX_MAP_SCALE = 100000;

TerrainThread::TerrainThread(RasterTerrain &_terrain,
                             std::function<void()> &&_callback)
  :StandbyThread("Terrain"), terrain(_terrain),
   callback(std::move(_callback)) {}

void
TerrainThread::Trigger(const WindowProjection &projection)
{
  assert(projection.IsValid());

  /* Rely on the overview cache when zoomed out; loading fine tiles
     for a continental view is expensive and not useful for display. */
  if (projection.GetMapScale() >= FINE_TILE_MAX_MAP_SCALE)
    return;

  const std::lock_guard lock{mutex};

  const GeoPoint center = projection.GetGeoScreenCenter();
  const auto screen_width = projection.GetScreenWidthMeters();
  const auto radius = screen_width / 2;
  /* Skip until the view has moved enough relative to map scale.  A
     fixed 1 km threshold was too tight at cruise scales and woke the
     thread on almost every pan event. */
  const auto min_move = std::max(1000., screen_width * 0.25);
  if (last_center.IsValid() && last_radius >= radius &&
      last_center.DistanceS(center) < min_move)
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
