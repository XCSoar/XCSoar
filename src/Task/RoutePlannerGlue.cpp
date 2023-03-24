// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RoutePlannerGlue.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "Airspace/ActivePredicate.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Route/ReachFan.hpp"

void
RoutePlannerGlue::SetTerrain(const RasterTerrain *_terrain)
{
  terrain = _terrain;
  if (terrain) {
    RasterTerrain::Lease lease(*terrain);
    planner.Reset();
    planner.SetTerrain(&terrain->map);
  } else {
    planner.Reset();
    planner.SetTerrain(nullptr);
  }
}

void
RoutePlannerGlue::Synchronise(const Airspaces &master,
                              const ProtectedAirspaceWarningManager *warnings,
                              const AGeoPoint &origin,
                              const AGeoPoint &destination)
{
  /* ignore acked airspaces (if we have an AirspaceWarningManager) */
  const auto predicate =
    WrapAirspacePredicate(ActiveAirspacePredicate(warnings));

  planner.Synchronise(master, predicate, origin, destination);
}

bool
RoutePlannerGlue::Solve(const AGeoPoint &origin,
                        const AGeoPoint &destination,
                        const RoutePlannerConfig &config,
                        const int h_ceiling)
{
  RasterTerrain::Lease lease(*terrain);
  return planner.Solve(origin, destination, config, h_ceiling);
}

ReachFan
RoutePlannerGlue::SolveReach(const AGeoPoint &origin,
                             const RoutePlannerConfig &config,
                             const int h_ceiling, const bool do_solve,
                             const bool working) noexcept
{
  if (terrain) {
    RasterTerrain::Lease lease(*terrain);
    return planner.SolveReach(origin, config, h_ceiling, do_solve, working);
  } else {
    return planner.SolveReach(origin, config, h_ceiling, do_solve, working);
  }
}

GeoPoint
RoutePlannerGlue::Intersection(const AGeoPoint &origin,
                               const AGeoPoint &destination) const
{
  RasterTerrain::Lease lease(*terrain);
  return planner.Intersection(origin, destination);
}
