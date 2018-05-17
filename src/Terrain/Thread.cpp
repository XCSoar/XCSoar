/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Thread.hpp"
#include "RasterTerrain.hpp"
#include "Projection/WindowProjection.hpp"
#include "Thread/Util.hpp"

TerrainThread::TerrainThread(RasterTerrain &_terrain,
                             std::function<void()> &&_callback)
  :StandbyThread("Terrain"), terrain(_terrain),
   callback(std::move(_callback)) {}

void
TerrainThread::Trigger(const WindowProjection &projection)
{
  assert(projection.IsValid());

  const ScopeLock protect(mutex);

  GeoPoint center = projection.GetGeoScreenCenter();
  auto radius = projection.GetScreenWidthMeters() / 2;
  if (last_center.IsValid() && last_radius >= radius &&
      last_center.DistanceS(center) < 1000)
    return;

  next_center = center;
  next_radius = radius;
  StandbyThread::Trigger();
}

void
TerrainThread::Tick()
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
