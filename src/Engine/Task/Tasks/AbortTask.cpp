/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "AbortTask.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Navigation/Aircraft.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "TaskSolvers/TaskSolution.hpp"
#include "Task/TaskEvents.hpp"
#include "Waypoint/WaypointVisitor.hpp"
#include "Util/queue.hpp"

const unsigned AbortTask::max_abort = 10; 
const fixed AbortTask::min_search_range(50000.0);
const fixed AbortTask::max_search_range(100000.0);

AbortTask::AbortTask(TaskEvents &_task_events, const TaskBehaviour &_task_behaviour,
                     const GlidePolar &_glide_polar, const Waypoints &wps)
  :UnorderedTask(ABORT, _task_events, _task_behaviour, _glide_polar),
   waypoints(wps),
   intersection_test(NULL),
   active_waypoint(0),
   polar_safety(_glide_polar)
{
  task_points.reserve(32);
}

AbortTask::~AbortTask()
{
  Clear();
}

void
AbortTask::SetTaskBehaviour(const TaskBehaviour &tb)
{
  UnorderedTask::SetTaskBehaviour(tb);

  for (auto i = task_points.begin(), end = task_points.end(); i != end; ++i)
    i->SetTaskBehaviour(tb);
}

void 
AbortTask::SetActiveTaskPoint(unsigned index)
{
  if (index < task_points.size()) {
    active_task_point = index;
    active_waypoint = task_points[index].GetWaypoint().id;
  }
}

TaskWaypoint*
AbortTask::GetActiveTaskPoint() const
{
  if (active_task_point < task_points.size())
    // XXX eliminate this deconst hack
    return const_cast<AlternateTaskPoint *>(&task_points[active_task_point]);

  return NULL;
}

bool
AbortTask::IsValidTaskPoint(int index_offset) const
{
  unsigned index = active_task_point + index_offset;
  return (index < task_points.size());
}

unsigned
AbortTask::TaskSize() const
{
  return task_points.size();
}

void
AbortTask::Clear()
{
  task_points.clear();
  reachable_landable = false;
}

fixed
AbortTask::GetAbortRange(const AircraftState &state) const
{
  // always scan at least min range or approx glide range
  return min(max_search_range,
             max(min_search_range, state.altitude * polar_safety.GetBestLD()));
}

GlidePolar
AbortTask::GetSafetyPolar() const
{
  const fixed mc_safety = task_behaviour.safety_mc;
  GlidePolar polar = glide_polar;
  polar.SetMC(mc_safety);
  return polar;
}

void
AbortTask::UpdatePolar(const SpeedVector& wind)
{
  // glide_polar for task
  polar_safety = GetSafetyPolar();
}

bool
AbortTask::IsTaskFull() const
{
  return task_points.size() >= max_abort;
}

/** Function object used to rank waypoints by arrival time */
struct AbortRank :
  public std::binary_function<AbortTask::Alternate, AbortTask::Alternate, bool>
{
  /** Condition, ranks by arrival time */
  bool operator()(const AbortTask::Alternate& x, 
                  const AbortTask::Alternate& y) const {
    return x.solution.time_elapsed + x.solution.time_virtual >
           y.solution.time_elapsed + y.solution.time_virtual;
  }
};

gcc_pure
static bool
IsReachable(const GlideResult &result, bool final_glide)
{
  return final_glide
    ? result.IsFinalGlide()
    : result.IsAchievable();
}

bool
AbortTask::FillReachable(const AircraftState &state,
                         AlternateVector &approx_waypoints,
                         const GlidePolar &polar, bool only_airfield,
                         bool final_glide, bool safety)
{
  if (IsTaskFull() || approx_waypoints.empty())
    return false;

  const AGeoPoint p_start(state.location, state.altitude);

  bool found_final_glide = false;
  reservable_priority_queue<Alternate, AlternateVector, AbortRank> q;
  q.reserve(32);

  for (auto v = approx_waypoints.begin(); v != approx_waypoints.end();) {
    if (only_airfield && !v->waypoint.IsAirport()) {
      ++v;
      continue;
    }

    UnorderedTaskPoint t(v->waypoint, task_behaviour);
    GlideResult result =
        TaskSolution::GlideSolutionRemaining(t, state, polar);
    /* calculate time_virtual, which is needed by AbortRank */
    result.CalcVInvSpeed(polar.GetInvMC());

    if (IsReachable(result, final_glide)) {
      bool intersects = false;
      const bool is_reachable_final = IsReachable(result, true);

      if (intersection_test && final_glide && is_reachable_final)
        intersects = intersection_test->Intersects(
            AGeoPoint(v->waypoint.location, result.min_height));

      if (!intersects) {
        q.push(Alternate(v->waypoint, result));
        // remove it since it's already in the list now      
        approx_waypoints.erase(v);

        if (is_reachable_final)
          found_final_glide = true;

        continue; // skip incrementing v since we just erased it
      }
    } 

    ++v;
  }

  while (!q.empty() && !IsTaskFull()) {
    const Alternate top = q.top();
    task_points.push_back(AlternateTaskPoint(top.waypoint, task_behaviour,
                                             top.solution));

    const int i = task_points.size() - 1;
    if (task_points[i].GetWaypoint().id == active_waypoint)
      active_task_point = i;

    q.pop();
  }

  return found_final_glide;
}

/**
 * Class to build vector from visited waypoints.
 * Intended to be used temporarily.
 */
class WaypointVisitorVector: public WaypointVisitor
{
public:
  /**
   * Constructor
   *
   * @param wpv Vector to add to
   *
   * @return Initialised object
   */
  WaypointVisitorVector(AbortTask::AlternateVector& wpv):vector(wpv) {}

  /**
   * Visit method, adds result to vector
   *
   * @param wp Waypoint that is visited
   */
  void Visit(const Waypoint& wp) {
    if (wp.IsLandable())
      vector.push_back(wp);
  }

private:
  AbortTask::AlternateVector &vector;
};

void 
AbortTask::ClientUpdate(const AircraftState &state_now, bool reachable)
{
  // nothing to do here, it's specialisations that may use this
}

bool 
AbortTask::UpdateSample(const AircraftState &state, bool full_update)
{
  UpdatePolar(state.wind);
  Clear();

  const unsigned active_waypoint_on_entry =
      is_active ? active_waypoint : (unsigned)-1;

  active_task_point = 0; // default to best result if can't find user-set one 

  AlternateVector approx_waypoints; 
  approx_waypoints.reserve(128);

  WaypointVisitorVector wvv(approx_waypoints);
  waypoints.VisitWithinRange(state.location, GetAbortRange(state), wvv);
  if (approx_waypoints.empty()) {
    /** @todo increase range */
    return false;
  }

  // lookup the appropriate polar to use
  const GlidePolar* mode_polar;
  switch (task_behaviour.route_planner.reach_polar_mode) {
  case RoutePlannerConfig::rpmTask:
    mode_polar = &glide_polar;
    // make copy to avoid waste
    break;
  case RoutePlannerConfig::rpmSafety:
    mode_polar = &polar_safety;
    break;
  default:
    assert(false);
    return false;
  }
  assert(mode_polar);

  // sort by alt difference

  // first try with final glide only
  reachable_landable |=  FillReachable(state, approx_waypoints, *mode_polar,
                                       true, true, true);
  reachable_landable |=  FillReachable(state, approx_waypoints, *mode_polar,
                                       false, true, true);

  // inform clients that the landable reachable scan has been performed 
  ClientUpdate(state, true);

  // now try without final glide constraint and not preferring airports
  FillReachable(state, approx_waypoints, *mode_polar, false, false, false);

  // inform clients that the landable unreachable scan has been performed 
  ClientUpdate(state, false);

  if (task_points.size()) {
    const TaskWaypoint &task_point = task_points[active_task_point];
    active_waypoint = task_point.GetWaypoint().id;
    if (is_active && (active_waypoint_on_entry != active_waypoint)) {
      task_events.ActiveChanged(task_point);
      return true;
    }
  }

  return false; // nothing to do
}

bool 
AbortTask::CheckTransitions(const AircraftState &, const AircraftState&)
{
  // nothing to do
  return false;
}

void 
AbortTask::AcceptTaskPointVisitor(TaskPointConstVisitor& visitor, bool reverse) const
{
  if (!reverse) {
    for (auto i = task_points.begin(), end = task_points.end(); i != end; ++i)
      visitor.Visit((const TaskPoint &)*i);
  } else {
    for (auto i = task_points.rbegin(), end = task_points.rend(); i != end; ++i)
      visitor.Visit((const TaskPoint &)*i);
  }
}

void
AbortTask::Reset()
{
  Clear();
  UnorderedTask::Reset();
}

const Waypoint *
AbortTask::GetHome() const
{
  return waypoints.GetHome();
}

GeoVector 
AbortTask::GetHomeVector(const AircraftState &state) const
{
  const Waypoint *home_waypoint = GetHome();
  if (home_waypoint)
    return GeoVector(state.location, home_waypoint->location);

  return GeoVector(fixed_zero);
}

GeoPoint 
AbortTask::GetTaskCenter(const GeoPoint& fallback_location) const
{
  if (task_points.empty())
    return fallback_location;

  TaskProjection task_projection;
  for (unsigned i = 0; i < task_points.size(); ++i) {
    const GeoPoint location = task_points[i].GetLocation();
    if (i == 0)
      task_projection.reset(location);
    else
      task_projection.scan_location(location);
  }
  task_projection.update_fast();
  return task_projection.get_center();
}

fixed 
AbortTask::GetTaskRadius(const GeoPoint& fallback_location) const
{ 
  if (task_points.empty())
    return fixed_zero;

  TaskProjection task_projection;
  for (unsigned i = 0; i < task_points.size(); ++i) {
    const GeoPoint location = task_points[i].GetLocation();
    if (i == 0)
      task_projection.reset(location);
    else
      task_projection.scan_location(location);
  }
  task_projection.update_fast();
  return task_projection.ApproxRadius();
}
