// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Factory.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Terrain/RasterTerrain.hpp"

#include <vector>

bool
WaypointFactory::FallbackElevation(Waypoint &waypoint) const noexcept
{
  if (terrain != nullptr) {
    // Load waypoint altitude from terrain
    const auto h = terrain->GetTerrainHeight(waypoint.location);
    if (!h.IsSpecial()) {
      waypoint.elevation = h.GetValue();
      waypoint.elevation_source = Waypoint::ElevationSource::TERRAIN;
      return true;
    }
  }

  return false;
}

/**
 * May the elevation of this waypoint be looked up in the terrain?
 */
static constexpr bool
UsesTerrainElevation(const Waypoint &waypoint) noexcept
{
  switch (waypoint.elevation_source) {
  case Waypoint::ElevationSource::NONE:
  case Waypoint::ElevationSource::TERRAIN:
    return true;

  case Waypoint::ElevationSource::FILE:
    return false;
  }

  return false;
}

void
UpdateTerrainElevations(Waypoints &waypoints,
                        const RasterTerrain &terrain) noexcept
{
  /* collect first: Waypoints::Replace() changes the tree being
     iterated */
  std::vector<WaypointPtr> outdated;
  for (const auto &waypoint : waypoints)
    if (UsesTerrainElevation(*waypoint))
      outdated.push_back(waypoint);

  const WaypointFactory factory(WaypointOrigin::NONE, 0, &terrain);

  for (const auto &waypoint : outdated) {
    Waypoint updated = *waypoint;
    updated.elevation_source = Waypoint::ElevationSource::NONE;
    factory.FallbackElevation(updated);

    if (updated.elevation_source != waypoint->elevation_source ||
        (updated.HasElevation() && updated.elevation != waypoint->elevation))
      waypoints.Replace(waypoint, std::move(updated));
  }

  waypoints.Optimise();
}
