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
  :terrain(NULL), m_planner(0), m_reach_polar_mode(RoutePlannerConfig::rpmTask)
#ifndef PLANNER_SET
  , m_unique(50000)
#endif
{
  reset();
}

void
RoutePlanner::reset()
{
  origin_last = AFlatGeoPoint(0, 0, 0);
  destination_last = AFlatGeoPoint(0, 0, 0);
  dirty = true;
  solution_route.clear();
  m_planner.clear();
  m_unique.clear();
  h_min = (short)-1;
  h_max = 0;
  m_search_hull.clear();
  reach.reset();
}

void
RoutePlanner::get_solution(Route &route) const
{
  route = solution_route;
}

bool
RoutePlanner::solve_reach(const AGeoPoint &origin, const bool do_solve)
{
  return reach.solve(origin, rpolars_reach, terrain, do_solve);
}

bool
RoutePlanner::solve(const AGeoPoint &origin, const AGeoPoint &destination,
                    const RoutePlannerConfig &config, const short h_ceiling)
{
  on_solve(origin, destination);
  rpolars_route.set_config(config, std::max(destination.altitude, origin.altitude),
                           h_ceiling);
  rpolars_reach.set_config(config, std::max(destination.altitude, origin.altitude),
                           h_ceiling);

  m_reach_polar_mode = config.reach_polar_mode;

  {
    const AFlatGeoPoint s_origin(task_projection.project(origin),
                                 origin.altitude);

    const AFlatGeoPoint s_destination(task_projection.project(destination),
                                      destination.altitude);

    if (!(s_origin == origin_last) || !(s_destination == destination_last))
      dirty = true;

    if (is_trivial())
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

  m_search_hull.clear();
  m_search_hull.push_back(SearchPoint(origin_last, task_projection));

  RoutePoint start = origin_last;
  m_astar_goal = destination_last;

  RouteLink e_test(start, m_astar_goal, task_projection);
  if (e_test.is_short())
    return false;
  if (!rpolars_route.achievable(e_test))
    return false;

  count_dij = 0;
  count_airspace = 0;
  count_terrain = 0;
  count_supressed = 0;

  bool retval = false;
  m_planner.restart(start);

  unsigned best_d = UINT_MAX;

  while (!m_planner.empty()) {
    const RoutePoint node = m_planner.pop();

    h_min = std::min(h_min, node.altitude);
    h_max = std::max(h_max, node.altitude);

    bool is_final = (node == m_astar_goal);
    if (is_final) {
      if (!retval)
        best_d = UINT_MAX;
      retval = true;
    }

    if (is_final) // @todo: allow fallback if failed
    { // copy improving solutions
      Route this_solution;
      unsigned d = find_solution(node, this_solution);
      if (d < best_d) {
        best_d = d;
        solution_route = this_solution;
      }
    }

    if (retval)
      break; // want top solution only

    // shoot for final
    RouteLink e(node, m_astar_goal, task_projection);
    if (set_unique(e))
      add_edges(e);

    while (!m_links.empty()) {
      add_edges(m_links.front());
      m_links.pop();
    }

  }

  count_unique = m_unique.size();

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

  m_planner.clear();
  m_unique.clear();
  // m_search_hull.clear();
  return retval;
}

unsigned
RoutePlanner::find_solution(const RoutePoint &final, Route &this_route) const
{
  // we are iterating from goal (aircraft) backwards to start (target)

  RoutePoint p(final);
  RoutePoint p_last(p);
  bool finished = false;

  this_route.insert(this_route.begin(),
                    AGeoPoint(task_projection.unproject(p), p.altitude));

  do {
    p_last = p;
    p = m_planner.get_predecessor(p);

    if (p == p_last) {
      finished = true;
      continue;
    }

    if (p.altitude < p_last.altitude &&
        !((FlatGeoPoint)p == (FlatGeoPoint)p_last)) {
      // create intermediate point for part cruise, part glide

      const RouteLink l(p, p_last, task_projection);
      const short vh = rpolars_route.calc_vheight(l);
      assert(vh>0);
      if (vh > p_last.altitude - p.altitude) { // climb was cut off
        const fixed f = (fixed)(p_last.altitude - p.altitude) / vh;
        const GeoPoint gp(task_projection.unproject(p));
        const GeoPoint gp_last(task_projection.unproject(p_last));
        const AGeoPoint gp_int(gp.interpolate(gp_last, f), p_last.altitude);
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

  return m_planner.get_node_value(final).h;
}

bool
RoutePlanner::link_cleared(const RouteLink &e)
{
  const bool is_final = (e.second == m_astar_goal);

  if (!rpolars_route.achievable(e, true))
    return false;

  if (!((FlatGeoPoint)e.second == m_astar_goal))
    assert(e.second.altitude >= e.first.altitude);

  const unsigned g = rpolars_route.calc_time(e);
  if (g == UINT_MAX)
    // not achievable
    return false;

  const RouteLink e_rem(e.second, m_astar_goal, task_projection);
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

  m_planner.reserve(ASTAR_QUEUE_SIZE);
  m_planner.link(e.second, e.first, v);
  return true;
}

bool
RoutePlanner::set_unique(const RouteLinkBase &e)
{
  if (m_unique.find(e) == m_unique.end()) {
    m_unique.insert(e);
    return true;
  }

  count_supressed++;
  return false;
}

void
RoutePlanner::add_candidate(const RouteLinkBase& e)
{
  if (e.is_short())
    return;
  if (!set_unique(e))
    return;

  const RouteLink c_link =
      rpolars_route.generate_intermediate(e.first, e.second, task_projection);

  m_links.push(c_link);
}

void
RoutePlanner::add_candidate(const RouteLink &e)
{
  if (!set_unique(e))
    return;

  m_links.push(e);
}

void
RoutePlanner::add_shortcut(const RoutePoint &node)
{
  const RoutePoint previous = m_planner.get_predecessor(node);
  if (previous == node)
    return;

  RoutePoint pre = previous;
  bool ok = true;
  do {
    RoutePoint pre_new = m_planner.get_predecessor(pre);
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
  const short vh = rpolars_route.calc_vheight(r_shortcut);
  if (!rpolars_route.can_climb())
    r_shortcut.second.altitude = r_shortcut.first.altitude + vh;

  if (check_clearance(r_shortcut, inx))
    link_cleared(r_shortcut);
}

void
RoutePlanner::add_edges(const RouteLink &e)
{
  const bool this_short = e.is_short();
  RoutePoint inx;
  if (!check_clearance(e, inx)) {
    if (!this_short)
      add_nearby(e);

    return;
  }

  if (!rpolars_route.achievable(e))
    return;

  if (!this_short)
    link_cleared(e);

  add_shortcut(e.second);
}

void
RoutePlanner::update_polar(const GlidePolar &task_polar,
                           const GlidePolar &safety_polar,
                           const SpeedVector &wind)
{
  rpolars_route.initialise(task_polar, wind);
  switch (m_reach_polar_mode) {
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
RoutePlanner::check_clearance_terrain(const RouteLink &e, RoutePoint& inp) const
{
  if (!terrain || !terrain->isMapLoaded())
    return true;

  count_terrain++;
  return rpolars_route.check_clearance(e, terrain, task_projection, inp);
}

void
RoutePlanner::add_nearby_terrain_sweep(const RoutePoint& p,
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
  if (!hull_extended(link_divert.second))
    return;

  add_candidate(link_divert);
}

void
RoutePlanner::add_nearby_terrain(const RoutePoint &p, const RouteLink& e)
{
  RouteLink c_link(e.first, p, task_projection);

  // dont process at all if too short
  if (c_link.is_short())
    return;

  // give secondary intersect method a chance to catch earlier intercept
  if (!check_secondary(c_link))
    return;

  // got this far, only process if first time here
  if (!set_unique(c_link))
    return;

  // add deflecting paths to get around obstacle
  if (positive(c_link.d)) {
    const RouteLinkBase end(e.first, m_astar_goal);
    if ((FlatGeoPoint)e.second == (FlatGeoPoint)m_astar_goal) {
      // if this link was shooting directly for goal, try both directions
      add_nearby_terrain_sweep(p, e, 1);
      add_nearby_terrain_sweep(p, e, -1);
    } else if (e.dot(end) > 0) {
      // this link was already deflecting, so keep deflecting in same direction
      // until we get 90 degrees to goal (no backtracking)
      add_nearby_terrain_sweep(p, e, e.cross(end) > 0 ? 1 : -1);
    }
  }

  if (!rpolars_route.achievable(c_link))
    return; // cant reach this

  // add clearance to intercept path
  link_cleared(c_link);

  // and add a shortcut option to this intercept point
  add_shortcut(c_link.second);

  //  RoutePoint dummy;
  //  assert(check_clearance(c_link, dummy));
}

void
RoutePlanner::on_solve(const AGeoPoint &origin, const AGeoPoint &destination)
{
  task_projection.reset(origin);
  task_projection.update_fast();
}

bool
RoutePlanner::hull_extended(const RoutePoint &p)
{
  if (m_search_hull.IsInside(p))
    return false;

  SearchPoint ps(p, task_projection);
  m_search_hull.push_back(ps);
  m_search_hull.PruneInterior();
  return true;
}

bool
RoutePlanner::intersection(const AGeoPoint& origin,
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
