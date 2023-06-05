// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RoutePlanner.hpp"
#include "ReachResult.hpp"
#include "Geo/Flat/FlatProjection.hpp"

RoutePlanner::RoutePlanner() noexcept
{
  Reset();
}

void
RoutePlanner::Reset() noexcept
{
  origin_last = AFlatGeoPoint(0, 0, 0);
  destination_last = AFlatGeoPoint(0, 0, 0);
  dirty = true;
  solution_route.clear();
  planner.Clear();
  unique_links.clear();
  h_min = -1;
  h_max = 0;
  search_hull.clear();
}

bool
RoutePlanner::Solve(const AGeoPoint &origin, const AGeoPoint &destination,
                    const RoutePlannerConfig &config, const int h_ceiling) noexcept
{
  OnSolve(origin, destination);
  rpolars_route.SetConfig(config, std::max(destination.altitude, origin.altitude),
                          h_ceiling);

  {
    const AFlatGeoPoint s_origin(projection.ProjectInteger(origin),
                                 origin.altitude);

    const AFlatGeoPoint s_destination(projection.ProjectInteger(destination),
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

  if (!rpolars_route.IsTerrainEnabled() && !rpolars_route.IsAirspaceEnabled())
    return false; // trivial

  search_hull.clear();
  search_hull.emplace_back(origin_last, projection);

  RoutePoint start = origin_last;
  astar_goal = destination_last;

  RouteLink e_test(start, astar_goal, projection);
  if (e_test.IsShort())
    return false;
  if (!rpolars_route.IsAchievable(e_test))
    return false;

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
    RouteLink e(node, astar_goal, projection);
    if (IsSetUnique(e))
      AddEdges(e);

    while (!links.empty()) {
      AddEdges(links.front());
      links.pop();
    }

  }

  if (retval) {
    // correct solution for rounding
    assert(solution_route.size()>=2);
    for (auto &i : solution_route) {
      FlatGeoPoint p(projection.ProjectInteger(i));
      if (p == origin_last) {
        i = AGeoPoint(origin, i.altitude);
      } else if (p == destination_last) {
        i = AGeoPoint(destination, i.altitude);
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
RoutePlanner::FindSolution(const RoutePoint &final_point,
                           Route &this_route) const noexcept
{
  // we are iterating from goal (aircraft) backwards to start (target)

  RoutePoint p(final_point);
  RoutePoint p_last(p);
  bool finished = false;

  this_route.insert(this_route.begin(),
                    AGeoPoint(projection.Unproject(p), p.altitude));

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

      const RouteLink l(p, p_last, projection);
      const double vh = rpolars_route.CalcVHeight(l);
      assert(vh > 0);
      if (vh > p_last.altitude - p.altitude) { // climb was cut off
        const auto f = (p_last.altitude - p.altitude) / vh;
        const auto gp = projection.Unproject(p);
        const auto gp_last = projection.Unproject(p_last);
        const AGeoPoint gp_int(gp.Interpolate(gp_last, f), p_last.altitude);
        this_route.insert(this_route.begin(), gp_int);
        // @todo: assert check_clearance?
      }
    } else if (p.altitude > p_last.altitude) {
      // create intermediate point for jump at end
      const AGeoPoint gp_int(projection.Unproject(p_last), p.altitude);
      this_route.insert(this_route.begin(), gp_int);
    }

    this_route.insert(this_route.begin(),
                      AGeoPoint(projection.Unproject(p), p.altitude));
    // @todo: assert check_clearance
  } while (!finished);

  return planner.GetNodeValue(final_point).h;
}

bool
RoutePlanner::LinkCleared(const RouteLink &e) noexcept
{
  const bool is_final = (e.second == astar_goal);

  if (!rpolars_route.IsAchievable(e, true))
    return false;

  if (!((FlatGeoPoint)e.second == astar_goal))
    assert(e.second.altitude >= e.first.altitude);

  const unsigned g = rpolars_route.CalcTime(e);
  if (g == UINT_MAX)
    // not achievable
    return false;

  const RouteLink e_rem(e.second, astar_goal, projection);
  if (!rpolars_route.IsAchievable(e_rem))
    return false;

  const unsigned h = rpolars_route.CalcTime(e_rem);
  if (h == UINT_MAX)
    // not achievable
    return false;

  assert(!(e.first==e.second));

  AStarPriorityValue v((is_final ? RoutePolars::RoundTime(g+h) : g),
                       (is_final ? 0 : RoutePolars::RoundTime(h)));
  // add one to tie-break towards lower number of links

  planner.Reserve(planner.DEFAULT_QUEUE_SIZE);
  planner.Link(e.second, e.first, v);
  return true;
}

bool
RoutePlanner::IsSetUnique(const RouteLinkBase &e) noexcept
{
  return unique_links.insert(e).second;
}

void
RoutePlanner::AddCandidate(const RouteLinkBase &e) noexcept
{
  if (e.IsShort())
    return;
  if (!IsSetUnique(e))
    return;

  const RouteLink c_link =
      rpolars_route.GenerateIntermediate(e.first, e.second, projection);

  links.push(c_link);
}

void
RoutePlanner::AddCandidate(const RouteLink &e) noexcept
{
  if (!IsSetUnique(e))
    return;

  links.push(e);
}

void
RoutePlanner::AddShortcut(const RoutePoint &node) noexcept
{
  const RoutePoint previous = planner.GetPredecessor(node);
  if (previous == node)
    return;

  RoutePoint pre = previous;
  bool ok = true;
  do {
    RoutePoint pre_new = planner.GetPredecessor(pre);
    if ((FlatGeoPoint)pre_new != (FlatGeoPoint)previous)
      ok = false;
    if (pre_new == pre)
      return;
    pre = pre_new;
  } while (ok);

  RouteLink r_shortcut(pre, node, projection);
  if (pre.altitude > node.altitude)
    return;

  assert(pre.altitude <= node.altitude);

  const int vh = rpolars_route.CalcVHeight(r_shortcut);
  if (!rpolars_route.CanClimb())
    r_shortcut.second.altitude = r_shortcut.first.altitude + vh;

  if (IsClear(r_shortcut))
    LinkCleared(r_shortcut);
}

void
RoutePlanner::AddEdges(const RouteLink &e) noexcept
{
  const bool this_short = e.IsShort();

  if (!IsClear(e)) {
    if (!this_short)
      AddNearby(e);

    return;
  }

  if (!rpolars_route.IsAchievable(e))
    return;

  if (!this_short)
    LinkCleared(e);

  AddShortcut(e.second);
}

void
RoutePlanner::UpdatePolar(const GlideSettings &settings,
                          const RoutePlannerConfig &config,
                          const GlidePolar &task_polar,
                          const SpeedVector &wind) noexcept
{
  rpolars_route.SetConfig(config);
  rpolars_route.Initialise(settings, task_polar, wind);
}

void
RoutePlanner::OnSolve(const AGeoPoint &origin,
                      [[maybe_unused]] const AGeoPoint &destination) noexcept
{
  projection.SetCenter(origin);
}

bool
RoutePlanner::IsHullExtended(const RoutePoint &p) noexcept
{
  if (search_hull.IsInside(p))
    return false;

  search_hull.emplace_back(p, projection);
  search_hull.PruneInterior();
  return true;
}
