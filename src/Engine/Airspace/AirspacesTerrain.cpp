// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspaces.hpp"
#include "Terrain/RasterTerrain.hpp"

void
Airspaces::SetGroundLevels(const RasterTerrain &terrain) noexcept
{
  for (auto &v : QueryAll()) {
    // If we don't need the ground level we don't have to calculate it
    if (!v.NeedGroundLevel())
      continue;

    FlatGeoPoint c_flat = v.GetCenter();
    GeoPoint g = task_projection.Unproject(c_flat);
    v.SetGroundLevel(terrain.GetTerrainHeight(g).GetValueOr0());
  }
}

