/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "RoutePlanner.hpp"
#include "Terrain/RasterMap.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Math/FastMath.h"

RoutePlanner::RoutePlanner()
  :terrain(NULL), planner(0), reach_polar_mode(RoutePlannerConfig::rpmTask)
#ifndef PLANNER_SET
  , unique_links(50000)
#endif
{
  Reset();
}

void
RoutePlanner::Reset()
{
  origin_last = AFlatGeoPoint(0, 0, RoughAltitude(0));
  destination_last = AFlatGeoPoint(0, 0, RoughAltitude(0));
  dirty = true;
  solution_route.clear();
  planner.Clear();
  unique_links.clear();
  h_min = RoughAltitude(-1);
  h_max = RoughAltitude(0);
  search_hull.clear();
  reach.reset();
}

void
RoutePlanner::GetSolution(Route &route) const
{
  route = solution_route;
}

bool
RoutePlanner::SolveReach(const AGeoPoint &origin, const bool do_solve)
{
  return reach.solve(origin, rpolars_reach, terrain, do_solve);
}

bool
RoutePlanner::Solve(const AGeoPoint &origin, const AGeoPoint &destination,
                    const RoutePlannerConfig &config, const RoughAltitude h_ceiling)
{
  OnSolve(origin, destination);
  rpolars_route.set_config(config, std::max(destination.altitude, origin.altitude),
                           h_ceiling);
  rpolars_reach.set_config(config, std::max(destination.altitude, origin.altitude),
                           h_ceiling);

  reach_polar_mode = config.reach_polar_mode;

  {
    const AFlatGeoPoint s_origin(task_projection.project(origin),
                                 origin.altitude);

    const AFlatGeoPoint s_destination(task_projection.project(destination),
                                      destination.altitude);

    if (!(s_origin == origin_last) || !(s_destination == destination_last))
      dirty = true;

    if (IsTrivial())
      return false;

    dirty = false;
    origin_last = s_origin;
    destination_last = s_destination;

    h_min = std::min(s_origin.altitude, s_destination.altitude);
    h_max = rpolars_route.cruise_altitude;
  }

  solution_route.clear();
  solution_route.push_back(origin);
  solution_route.push_back(destination);

  if (!rpolars_route.terrain_enabled() && !rpolars_route.airspace_enabled())
    return false; // trivial

  search_hull.clear();
  search_hull.push_back(SearchPoint(origin_last, task_projection));

  RoutePoint start = origin_last;
  astar_goal = destination_last;

  RouteLink e_test(start, astar_goal, task_projection);
  if (e_test.is_short())
    return false;
  if (!rpolars_route.achievable(e_test))
    return false;

  count_dij = 0;
  count_airspace = 0;
  count_terrain = 0;
  count_supressed = 0;

  bool retval = false;
  planner.Restart(start);

  unsigned best_d = UINT_MAX;

  while (!planner.IsEmpty()) {
    const RoutePoint node = planner.Pop();

    h_min = std::min(h_min, node.altitude);
    h_max = std::max(h_max, node.altitude);

    bool is_final = (node == astar_goal);
    if (is_final) {
      if (!retval)
        best_d = UINT_MAX;
      retval = true;
    }

    if (is_final) // @todo: allow fallback if failed
    { // copy improving solutions
      Route this_solution;
      unsigned d = FindSolution(node, this_solution);
      if (d < best_d) {
        best_d = d;
        solution_route = this_solution;
      }
    }

    if (retval)
      break; // want top solution only

    // shoot for final
    RouteLink e(node, astar_goal, task_projection);
    if (IsSetUnique(e))
      AddEdges(e);

    while (!links.empty()) {
      AddEdges(links.front());
      links.pop();
    }

  }

  count_unique = unique_links.size();

  if (retval) {
    // correct solution for rounding
    assert(solution_route.size()>=2);
    for (unsigned i = 0; i < solution_route.size(); ++i) {
      FlatGeoPoint p(task_projection.project(solution_route[i]));
      if (p == origin_last) {
        solution_route[i] = AGeoPoint(origin, solution_route[i].altitude);
      } else if (p == destination_last) {
        solution_route[i] = AGeoPoint(destination, solution_route[i].altitude);
      }
    }

  } else {
    solution_route.clear();
    solution_route.push_back(origin);
    solution_route.push_back(destination);
  }

  planner.Clear();
  unique_links.clear();
  // m_search_hull.clear();
  return retval;
}

unsigned
RoutePlanner::FindSolution(const RoutePoint &final, Route &this_route) const
{
  // we are iterating from goal (aircraft) backwards to start (target)

  RoutePoint p(final);
  RoutePoint p_last(p);
  bool finished = false;

  this_route.insert(this_route.begin(),
                    AGeoPoint(task_projection.unproject(p), p.altitude));

  do {
    p_last = p;
    p = planner.GetPredecessor(p);

    if (p == p_last) {
      finished = true;
      continue;
    }

    if (p.altitude < p_last.altitude &&
        !((FlatGeoPoint)p == (FlatGeoPoint)p_last)) {
      // create intermediate point for part cruise, part glide

      const RouteLink l(p, p_last, task_projection);
      const RoughAltitude vh = rpolars_route.calc_vheight(l);
      assert(vh.IsPositive());
      if (vh > p_last.altitude - p.altitude) { // climb was cut off
        const fixed f = (p_last.altitude - p.altitude) / vh;
        const GeoPoint gp(task_projection.unproject(p));
        const GeoPoint gp_last(task_projection.unproject(p_last));
        const AGeoPoint gp_int(gp.Interpolate(gp_last, f), p_last.altitude);
        this_route.insert(this_route.begin(), gp_int);
        // @todo: assert check_clearance?
      }
    } else if (p.altitude > p_last.altitude) {
      // create intermediate point for jump at end
      const AGeoPoint gp_int(task_projection.unproject(p_last), p.altitude);
      this_route.insert(this_route.begin(), gp_int);
    }

    this_route.insert(this_route.begin(),
                      AGeoPoint(task_projection.unproject(p), p.altitude));
    // @todo: assert check_clearance
  } while (!finished);

  return planner.GetNodeValue(final).h;
}

bool
RoutePlanner::LinkCleared(const RouteLink &e)
{
  const bool is_final = (e.second == astar_goal);

  if (!rpolars_route.achievable(e, true))
    return false;

  if (!((FlatGeoPoint)e.second == astar_goal))
    assert(e.second.altitude >= e.first.altitude);

  const unsigned g = rpolars_route.calc_time(e);
  if (g == UINT_MAX)
    // not achievable
    return false;

  const RouteLink e_rem(e.second, astar_goal, task_projection);
  if (!rpolars_route.achievable(e_rem))
    return false;

  const unsigned h = rpolars_route.calc_time(e_rem);
  if (h == UINT_MAX)
    // not achievable
    return false;

  assert(!(e.first==e.second));

  count_dij++;
  AStarPriorityValue v((is_final ? RoutePolars::round_time(g+h) : g),
                       (is_final ? 0 : RoutePolars::round_time(h)));
  // add one to tie-break towards lower number of links

  planner.Reserve(ASTAR_QUEUE_SIZE);
  planner.Link(e.second, e.first, v);
  return true;
}

bool
RoutePlanner::IsSetUnique(const RouteLinkBase &e)
{
  if (unique_links.find(e) == unique_links.end()) {
    unique_links.insert(e);
    return true;
  }

  count_supressed++;
  return false;
}

void
RoutePlanner::AddCandidate(const RouteLinkBase& e)
{
  if (e.is_short())
    return;
  if (!IsSetUnique(e))
    return;

  const RouteLink c_link =
      rpolars_route.generate_intermediate(e.first, e.second, task_projection);

  links.push(c_link);
}

void
RoutePlanner::AddCandidate(const RouteLink &e)
{
  if (!IsSetUnique(e))
    return;

  links.push(e);
}

void
RoutePlanner::AddShortcut(const RoutePoint &node)
{
  const RoutePoint previous = planner.GetPredecessor(node);
  if (previous == node)
    return;

  RoutePoint pre = previous;
  bool ok = true;
  do {
    RoutePoint pre_new = planner.GetPredecessor(pre);
    if (!((FlatGeoPoint)pre_new == (FlatGeoPoint)previous))
      ok = false;
    if (pre_new == pre)
      return;
    pre = pre_new;
  } while (ok);

  RouteLink r_shortcut(pre, node, task_projection);
  if (pre.altitude > node.altitude)
    return;

  assert(pre.altitude <= node.altitude);

  RoutePoint inx;
  const RoughAltitude vh = rpolars_route.calc_vheight(r_shortcut);
  if (!rpolars_route.can_climb())
    r_shortcut.second.altitude = r_shortcut.first.altitude + vh;

  if (CheckClearance(r_shortcut, inx))
    LinkCleared(r_shortcut);
}

void
RoutePlanner::AddEdges(const RouteLink &e)
{
  const bool this_short = e.is_short();
  RoutePoint inx;
  if (!CheckClearance(e, inx)) {
    if (!this_short)
      AddNearby(e);

    return;
  }

  if (!rpolars_route.achievable(e))
    return;

  if (!this_short)
    LinkCleared(e);

  AddShortcut(e.second);
}

void
RoutePlanner::UpdatePolar(const GlidePolar &task_polar,
                           const GlidePolar &safety_polar,
                           const SpeedVector &wind)
{
  rpolars_route.initialise(task_polar, wind);
  switch (reach_polar_mode) {
  case RoutePlannerConfig::rpmTask:
    rpolars_reach = rpolars_route;
    glide_polar_reach = task_polar;
    // make copy to avoid waste
    break;
  case RoutePlannerConfig::rpmSafety:
    rpolars_reach.initialise(safety_polar, wind);
    glide_polar_reach = safety_polar;
    break;
  }
}

/*
  add_edges loop: only add edges in links originating from the node?
  - or do shortcut checks at end of add_edges loop
*/

bool
RoutePlanner::CheckClearanceTerrain(const RouteLink &e, RoutePoint& inp) const
{
  if (!terrain || !terrain->isMapLoaded())
    return true;

  count_terrain++;
  return rpolars_route.check_clearance(e, terrain, task_projection, inp);
}

void
RoutePlanner::AddNearbyTerrainSweep(const RoutePoint& p,
                                       const RouteLink &c_link, const int sign)
{
  // dont add if no distance
  if ((FlatGeoPoint)c_link.first == (FlatGeoPoint)p)
    return;

  // make short link neighbouring last intercept
  RouteLink link_divert = rpolars_route.neighbour_link(c_link.first, p,
                                                       task_projection, sign);

  // dont add directions 90 degrees away from target
  if (link_divert.dot(c_link) <= 0)
    return;

  // ensure the target is achievable due to climb constraints
  if (!rpolars_route.achievable(link_divert))
    return;

  // don't add if inside hull
  if (!IsHullExtended(link_divert.second))
    return;

  AddCandidate(link_divert);
}

void
RoutePlanner::AddNearbyTerrain(const RoutePoint &p, const RouteLink& e)
{
  RouteLink c_link(e.first, p, task_projection);

  // dont process at all if too short
  if (c_link.is_short())
    return;

  // give secondary intersect method a chance to catch earlier intercept
  if (!CheckSecondary(c_link))
    return;

  // got this far, only process if first time here
  if (!IsSetUnique(c_link))
    return;

  // add deflecting paths to get around obstacle
  if (positive(c_link.d)) {
    const RouteLinkBase end(e.first, astar_goal);
    if ((FlatGeoPoint)e.second == (FlatGeoPoint)astar_goal) {
      // if this link was shooting directly for goal, try both directions
      AddNearbyTerrainSweep(p, e, 1);
      AddNearbyTerrainSweep(p, e, -1);
    } else if (e.dot(end) > 0) {
      // this link was already deflecting, so keep deflecting in same direction
      // until we get 90 degrees to goal (no backtracking)
      AddNearbyTerrainSweep(p, e, e.cross(end) > 0 ? 1 : -1);
    }
  }

  if (!rpolars_route.achievable(c_link))
    return; // cant reach this

  // add clearance to intercept path
  LinkCleared(c_link);

  // and add a shortcut option to this intercept point
  AddShortcut(c_link.second);

  //  RoutePoint dummy;
  //  assert(check_clearance(c_link, dummy));
}

void
RoutePlanner::OnSolve(const AGeoPoint &origin, const AGeoPoint &destination)
{
  task_projection.reset(origin);
  task_projection.update_fast();
}

bool
RoutePlanner::IsHullExtended(const RoutePoint &p)
{
  if (search_hull.IsInside(p))
    return false;

  SearchPoint ps(p, task_projection);
  search_hull.push_back(ps);
  search_hull.PruneInterior();
  return true;
}

bool
RoutePlanner::Intersection(const AGeoPoint& origin,
                           const AGeoPoint& destination, GeoPoint& intx) const
{
  TaskProjection proj;
  proj.reset(origin);
  proj.update_fast();
  return rpolars_route.intersection(origin, destination, terrain, proj, intx);
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
  - tr1::hash function portability between compiler versions
  - AirspaceRoute synchronise method to disable/ignore airspaces that are
    acknowledged in the airspace warning manager.
  - more documentation
 */
