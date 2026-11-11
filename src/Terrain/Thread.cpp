// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Thread.hpp"
#include "RasterTerrain.hpp"
#include "Projection/WindowProjection.hpp"
#include "Projection/MapLod.hpp"
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
  const double scale = projection.GetScale();
  const double pixel_size = scale > 0 ? 1. / scale : 0;
  const unsigned lod = MapLod(projection.GetMapScale());

  const bool moved = !last_center.IsValid() ||
    last_center.DistanceS(center) >= 1000;
  const bool need_wider = radius > last_radius;
  const bool lod_changed = lod != last_lod;
  if (!moved && !need_wider && !lod_changed)
    return;

  next_center = center;
  next_radius = radius;
  next_pixel_size = pixel_size;
  next_lod = lod;
  StandbyThread::Trigger();
}

void
TerrainThread::Tick() noexcept
{
  SetIdlePriority(); // TODO: call only once

  bool again = true;
  unsigned lod = next_lod;
  GeoPoint center = GeoPoint::Invalid();
  double radius = 0;
  double pixel_size = 0;
  while (next_center.IsValid() && again && !IsStopped()) {
    center = next_center;
    radius = next_radius;
    pixel_size = next_pixel_size;
    lod = next_lod;

    {
      const ScopeUnlock unlock(mutex);
      again = terrain.UpdateTiles(center, radius, pixel_size);
    }

    last_center = center;
    last_radius = radius;
    last_lod = lod;
  }

  if (center.IsValid() && !IsStopped()) {
    const ScopeUnlock unlock(mutex);
    if (lod > 0) {
      const double f =
        MapLodRepresentativeMapScale(lod - 1) /
        MapLodRepresentativeMapScale(lod);
      terrain.UpdateTiles(center, radius * f, pixel_size * f);
    }
    if (lod + 1 < MAP_LOD_COUNT) {
      const double f =
        MapLodRepresentativeMapScale(lod + 1) /
        MapLodRepresentativeMapScale(lod);
      terrain.UpdateTiles(center, radius * f, pixel_size * f);
    }
  }

  /* notify the client */
  if (callback) {
    const ScopeUnlock unlock(mutex);
    callback();
  }
}
