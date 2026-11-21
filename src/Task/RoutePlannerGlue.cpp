// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RoutePlannerGlue.hpp"
#include "Airspace/ActivePredicate.hpp"
#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Route/ReachFan.hpp"

void
RoutePlannerGlue::SetTerrain([[maybe_unused]] const TerrainBase *_terrain)
{
  planner.Reset();
  /* Route/reach still sample RasterMap; COG never fills that cache. */
  planner.SetTerrain(nullptr);
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
  return planner.Solve(origin, destination, config, h_ceiling);
}

ReachFan
RoutePlannerGlue::SolveReach(const AGeoPoint &origin,
                             const RoutePlannerConfig &config,
                             const int h_ceiling, const bool do_solve,
                             const bool working) noexcept
{
  return planner.SolveReach(origin, config, h_ceiling, do_solve, working);
}

GeoPoint
RoutePlannerGlue::Intersection(const AGeoPoint &origin,
                               const AGeoPoint &destination) const
{
  return planner.Intersection(origin, destination);
}
