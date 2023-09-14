// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TerrainRoute.hpp"
#include "ReachResult.hpp"
#include "ReachFan.hpp"
#include "Terrain/RasterMap.hpp"

void
TerrainRoute::UpdatePolar(const GlideSettings &settings,
                          const RoutePlannerConfig &config,
                          const GlidePolar &task_polar,
                          const GlidePolar &safety_polar,
                          const SpeedVector &wind,
                          const int height_min_working) noexcept
{
  RoutePlanner::UpdatePolar(settings, config, task_polar, wind);

  switch (config.reach_polar_mode) {
  case RoutePlannerConfig::Polar::TASK:
    rpolars_reach = rpolars_route;
    // make copy to avoid waste
    break;
  case RoutePlannerConfig::Polar::SAFETY:
    rpolars_reach.SetConfig(config);
    rpolars_reach.Initialise(settings, safety_polar, wind);
    break;
  }

  rpolars_reach_working.SetConfig(config);
  rpolars_reach_working.Initialise(settings, task_polar, wind,
                                   height_min_working);
}

ReachFan
TerrainRoute::SolveReach(const AGeoPoint &origin,
                         const RoutePlannerConfig &config,
                         const int h_ceiling,
                         const bool do_solve,
                         const bool working) noexcept
{
  auto &rpolars = working ? rpolars_reach_working : rpolars_reach;
  rpolars.SetConfig(config, origin.altitude, h_ceiling);

  ReachFan reach;
  reach.Solve(origin, rpolars, terrain, do_solve);
  return reach;
}

/*
  @todo:
  - check wind directions are correct
  - check overflow/accuracy of slope factor in RasterTile::FirstIntersection
  - graphical feedback on flight mode of Route in GUI
  - ignore airspaces that start/end points are inside?
  - promote stable solutions with rounding of time value
  - adjustment to GlideSolution height/time in task manager according to path
    variation required for terrain/airspace avoidance
  - AirspaceRoute synchronise method to disable/ignore airspaces that are
    acknowledged in the airspace warning manager.
  - more documentation
 */

GeoPoint
TerrainRoute::Intersection(const AGeoPoint &origin,
                           const AGeoPoint &destination) const noexcept
{
  const FlatProjection proj(origin);
  return rpolars_route.Intersection(origin, destination, terrain, proj);
}

bool
TerrainRoute::IsClear(const RouteLink &e) const noexcept
{
  if (terrain == nullptr || !terrain->IsDefined())
    return true;

  auto inp = rpolars_route.CheckClearance(e, *terrain, projection);
  if (inp)
    m_inx_terrain = *inp;
  return !inp;
}

void
TerrainRoute::AddNearby(const RouteLink &e) noexcept
{
  AddNearbyTerrain(m_inx_terrain, e);
}

/*
  add_edges loop: only add edges in links originating from the node?
  - or do shortcut checks at end of add_edges loop
*/

void
TerrainRoute::AddNearbyTerrainSweep(const RoutePoint &p,
                                    const RouteLink &c_link,
                                    const int sign) noexcept
{
  // dont add if no distance
  if ((FlatGeoPoint)c_link.first == (FlatGeoPoint)p)
    return;

  // make short link neighbouring last intercept
  RouteLink link_divert = rpolars_route.NeighbourLink(c_link.first, p,
                                                      projection, sign);

  // dont add directions 90 degrees away from target
  if (link_divert.DotProduct(c_link) <= 0)
    return;

  // ensure the target is achievable due to climb constraints
  if (!rpolars_route.IsAchievable(link_divert))
    return;

  // don't add if inside hull
  if (!IsHullExtended(link_divert.second))
    return;

  AddCandidate(link_divert);
}

inline void
TerrainRoute::AddNearbyTerrain(const RoutePoint &p,
                               const RouteLink &e) noexcept
{
  RouteLink c_link(e.first, p, projection);

  // dont process at all if too short
  if (c_link.IsShort())
    return;

  // give secondary intersect method a chance to catch earlier intercept
  if (!CheckSecondary(c_link))
    return;

  // got this far, only process if first time here
  if (!IsSetUnique(c_link))
    return;

  // add deflecting paths to get around obstacle
  if (c_link.d > 0) {
    const RouteLinkBase end(e.first, astar_goal);
    if ((FlatGeoPoint)e.second == (FlatGeoPoint)astar_goal) {
      // if this link was shooting directly for goal, try both directions
      AddNearbyTerrainSweep(p, e, 1);
      AddNearbyTerrainSweep(p, e, -1);
    } else if (e.DotProduct(end) > 0) {
      // this link was already deflecting, so keep deflecting in same direction
      // until we get 90 degrees to goal (no backtracking)
      AddNearbyTerrainSweep(p, e, e.CrossProduct(end) > 0 ? 1 : -1);
    }
  }

  if (!rpolars_route.IsAchievable(c_link))
    return; // cant reach this

  // add clearance to intercept path
  LinkCleared(c_link);

  // and add a shortcut option to this intercept point
  AddShortcut(c_link.second);

  //  RoutePoint dummy;
  //  assert(check_clearance(c_link, dummy));
}
