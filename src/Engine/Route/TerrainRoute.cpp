/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "TerrainRoute.hpp"
#include "ReachResult.hpp"
#include "Terrain/RasterMap.hpp"

void
TerrainRoute::ClearReach() noexcept
{
  reach_terrain.Reset();
  reach_working.Reset();
}

void
TerrainRoute::Reset() noexcept
{
  RoutePlanner::Reset();
  ClearReach();
}

bool
TerrainRoute::SolveReachTerrain(const AGeoPoint &origin,
                                const RoutePlannerConfig &config,
                                const int h_ceiling,
                                const bool do_solve) noexcept
{
  rpolars_reach.SetConfig(config, origin.altitude, h_ceiling);

  return reach_terrain.Solve(origin, rpolars_reach, terrain, do_solve);
}

bool
TerrainRoute::SolveReachWorking(const AGeoPoint &origin,
                                const RoutePlannerConfig &config,
                                const int h_ceiling,
                                const bool do_solve) noexcept
{
  rpolars_reach_working.SetConfig(config, origin.altitude, h_ceiling);

  return reach_working.Solve(origin, rpolars_reach_working, terrain, do_solve);
}

std::optional<ReachResult>
TerrainRoute::FindPositiveArrival(const AGeoPoint &dest) const noexcept
{
  return reach_terrain.FindPositiveArrival(dest, rpolars_reach);
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

void
TerrainRoute::AcceptInRange(const GeoBounds &bounds,
                            FlatTriangleFanVisitor &visitor,
                            bool working) const noexcept
{
  if (working)
    reach_working.AcceptInRange(bounds, visitor);
  else
    reach_terrain.AcceptInRange(bounds, visitor);
}

GeoPoint
TerrainRoute::Intersection(const AGeoPoint &origin,
                           const AGeoPoint &destination) const noexcept
{
  const FlatProjection proj(origin);
  return rpolars_route.Intersection(origin, destination, terrain, proj);
}

std::optional<RoutePoint>
TerrainRoute::CheckClearance(const RouteLink &e) const noexcept
{
  auto inp = CheckClearanceTerrain(e);
  if (inp)
    m_inx_terrain = *inp;
  return inp;
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

std::optional<RoutePoint>
TerrainRoute::CheckClearanceTerrain(const RouteLink &e) const noexcept
{
  if (!terrain || !terrain->IsDefined())
    return std::nullopt;

  count_terrain++;
  return rpolars_route.CheckClearance(e, terrain, projection);
}

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

void
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
