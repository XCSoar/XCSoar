// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProtectedRoutePlanner.hpp"
#include "Engine/Route/ReachResult.hpp"

void
ProtectedRoutePlanner::SetTerrain(const RasterTerrain *terrain) noexcept
{
  const std::scoped_lock lock{route_mutex};
  route_planner.SetTerrain(terrain);
}

void
ProtectedRoutePlanner::SetPolars(const GlideSettings &settings,
                                 const RoutePlannerConfig &config,
                                 const GlidePolar &glide_polar,
                                 const GlidePolar &safety_polar,
                                 const SpeedVector &wind,
                                 const int height_min_working) noexcept
{
  const std::scoped_lock lock{route_mutex};
  route_planner.UpdatePolar(settings, config, glide_polar, safety_polar,
                            wind, height_min_working);
}

void
ProtectedRoutePlanner::SolveRoute(const AGeoPoint &dest,
                                  const AGeoPoint &start,
                                  const RoutePlannerConfig &config,
                                  const int h_ceiling) noexcept
{
  const std::scoped_lock lock{route_mutex};
  route_planner.Synchronise(airspaces, warnings, dest, start);
  route_planner.Solve(dest, start, config, h_ceiling);
}

void
ProtectedRoutePlanner::SolveReach(const AGeoPoint &origin,
                                  const RoutePlannerConfig &config,
                                  const int h_ceiling,
                                  const bool do_solve) noexcept
{
  /* these local variables help avoid locking both mutexes at the same
     time */
  ReachFan rt, rw;

  {
    const std::scoped_lock lock{route_mutex};
    rt = route_planner.SolveReach(origin, config, h_ceiling, do_solve, false);
    rw = route_planner.SolveReach(origin, config, h_ceiling, do_solve, true);
    rpolars_reach = route_planner.GetReachPolar();
  }

  /* we lock this mutex not during the expensive reach calculation,
     but only for moving the result to the mutex-protected fields */
  const std::scoped_lock lock{reach_mutex};
  reach_terrain = std::move(rt);
  reach_working = std::move(rw);
}

const FlatProjection
ProtectedRoutePlanner::GetTerrainReachProjection() const noexcept
{
  const std::scoped_lock lock{reach_mutex};
  return reach_terrain.GetProjection();
}

std::optional<ReachResult>
ProtectedRoutePlanner::FindPositiveArrival(const AGeoPoint &dest) const noexcept
{
  const std::scoped_lock lock{reach_mutex};
  return reach_terrain.FindPositiveArrival(dest, rpolars_reach);
}

void
ProtectedRoutePlanner::AcceptInRange(const GeoBounds &bounds,
                                     FlatTriangleFanVisitor &visitor,
                                     bool working) const noexcept
{
  const std::scoped_lock lock{reach_mutex};
  const auto &reach = working ? reach_working : reach_terrain;
  reach.AcceptInRange(bounds, visitor);
}

int
ProtectedRoutePlanner::GetTerrainBase() const noexcept
{
  const std::scoped_lock lock{reach_mutex};
  return reach_terrain.GetTerrainBase();
}
