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

AbortTask::AbortTask(TaskEvents &te, const TaskBehaviour &tb,
                     const GlidePolar &gp, const Waypoints &wps):
  UnorderedTask(ABORT, te, tb, gp),
  waypoints(wps),
  intersection_test(NULL),
  active_waypoint(0),
  polar_safety(gp)
{
  task_points.reserve(32);
}

AbortTask::~AbortTask()
{
  clear();
}

void
AbortTask::SetTaskBehaviour(const TaskBehaviour &tb)
{
  UnorderedTask::SetTaskBehaviour(tb);

  AlternateTaskVector::iterator end = task_points.end();
  for (AlternateTaskVector::iterator i = task_points.begin(); i != end; ++i)
    i->SetTaskBehaviour(tb);

}

void 
AbortTask::setActiveTaskPoint(unsigned index)
{
  if (index < task_points.size()) {
    activeTaskPoint = index;
    active_waypoint = task_points[index].get_waypoint().id;
  }
}

TaskWaypoint*
AbortTask::getActiveTaskPoint() const
{
  if (activeTaskPoint < task_points.size())
    // XXX eliminate this deconst hack
    return const_cast<AlternateTaskPoint *>(&task_points[activeTaskPoint]);

  return NULL;
}

bool
AbortTask::validTaskPoint(const int index_offset) const
{
  unsigned index = activeTaskPoint + index_offset;
  return (index < task_points.size());
}

unsigned
AbortTask::task_size() const
{
  return task_points.size();
}

void
AbortTask::clear()
{
  task_points.clear();
  m_landable_reachable = false;
}

fixed
AbortTask::abort_range(const AIRCRAFT_STATE &state) const
{
  // always scan at least min range or approx glide range
  return min(max_search_range,
             max(min_search_range, state.NavAltitude * polar_safety.GetBestLD()));
}

GlidePolar
AbortTask::get_safety_polar() const
{
  const fixed mc_safety = task_behaviour.safety_mc;
  GlidePolar polar = glide_polar;
  polar.SetMC(mc_safety);
  return polar;
}

void
AbortTask::update_polar(const SpeedVector& wind)
{
  // glide_polar for task
  polar_safety = get_safety_polar();
}

bool
AbortTask::task_full() const
{
  return (task_points.size() >= max_abort);
}

/**
 * Function object used to rank waypoints by arrival time
 */
struct AbortRank :
  public std::binary_function<AbortTask::Alternate, AbortTask::Alternate, bool>
{
  /**
   * Condition, ranks by arrival time 
   */
  bool operator()(const AbortTask::Alternate& x, 
                  const AbortTask::Alternate& y) const {
    return x.solution.TimeElapsed + x.solution.TimeVirtual >
           y.solution.TimeElapsed + y.solution.TimeVirtual;
  }
};

bool 
AbortTask::is_reachable(const GlideResult &result,
                        const bool final_glide) const 
{
  return !positive(result.Vector.Distance) || 
    (!negative(result.TimeElapsed) && result.glide_reachable(final_glide));
}

bool
AbortTask::fill_reachable(const AIRCRAFT_STATE &state,
                          AlternateVector &approx_waypoints,
                          const GlidePolar &polar,
                          const bool only_airfield,
                          const bool final_glide,
                          const bool safety)
{
  if (task_full() || approx_waypoints.empty())
    return false;

  const AGeoPoint p_start (state.Location, (short)state.NavAltitude);

  bool found_final_glide = false;
  reservable_priority_queue<Alternate, AlternateVector, AbortRank> q;
  q.reserve(32);

  for (AlternateVector::iterator v = approx_waypoints.begin();
       v != approx_waypoints.end();) {
    if (only_airfield && !v->waypoint.IsAirport()) {
      ++v;
      continue;
    }

    UnorderedTaskPoint t(v->waypoint, task_behaviour);
    const GlideResult result = TaskSolution::glide_solution_remaining(t, state, polar);
    if (is_reachable(result, final_glide)) {
      bool intersects = false;
      const bool is_reachable_final = is_reachable(result, true);

      if (intersection_test && final_glide && is_reachable_final) {
        intersects = intersection_test->intersects(AGeoPoint(v->waypoint.Location,
                                                             result.MinHeight));
      }
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

  while (!q.empty() && !task_full()) {
    const Alternate top = q.top();
    task_points.push_back(AlternateTaskPoint(top.waypoint, task_behaviour,
                                             top.solution));

    const int i = task_points.size() - 1;
    if (task_points[i].get_waypoint().id == active_waypoint)
      activeTaskPoint = i;

    q.pop();
  }
  return found_final_glide;
}

/**
 * Class to build vector from visited waypoints.
 * Intended to be used temporarily.
 */
class WaypointVisitorVector: 
  public WaypointVisitor 
{
public:
  /**
   * Constructor
   *
   * @param wpv Vector to add to
   *
   * @return Initialised object
   */
  WaypointVisitorVector(AbortTask::AlternateVector& wpv):vector(wpv) {};

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
AbortTask::client_update(const AIRCRAFT_STATE &state_now,
                         const bool reachable)
{
  // nothing to do here, it's specialisations that may use this
}

bool 
AbortTask::update_sample(const AIRCRAFT_STATE &state, 
                         const bool full_update)
{
  update_polar(state.wind);
  clear();

  const unsigned active_waypoint_on_entry = is_active? active_waypoint: (unsigned)-1;
  activeTaskPoint = 0; // default to best result if can't find user-set one 

  AlternateVector approx_waypoints; 
  approx_waypoints.reserve(128);

  WaypointVisitorVector wvv(approx_waypoints);
  waypoints.visit_within_range(state.Location, abort_range(state), wvv);
  if (approx_waypoints.empty()) {
    /**
     * \todo
     * - increase range
     */
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
  m_landable_reachable|=  fill_reachable(state, approx_waypoints, *mode_polar, true, true, true);
  m_landable_reachable|=  fill_reachable(state, approx_waypoints, *mode_polar, false, true, true);

  // inform clients that the landable reachable scan has been performed 
  client_update(state, true);

  // now try without final glide constraint and not preferring airports
  fill_reachable(state, approx_waypoints, *mode_polar, false, false, false);

  // inform clients that the landable unreachable scan has been performed 
  client_update(state, false);

  if (task_points.size()) {
    const TaskWaypoint &task_point = task_points[activeTaskPoint];
    active_waypoint = task_point.get_waypoint().id;
    if (is_active && (active_waypoint_on_entry != active_waypoint))
      task_events.active_changed(task_point);
  }

  return false; // nothing to do
}


bool 
AbortTask::check_transitions(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&)
{
  // nothing to do
  return false;
}

void 
AbortTask::tp_CAccept(TaskPointConstVisitor& visitor, const bool reverse) const
{
  if (!reverse) {
    const AlternateTaskVector::const_iterator end = task_points.end();
    for (AlternateTaskVector::const_iterator i = task_points.begin();
         i != end; ++i)
      visitor.Visit((const TaskPoint &)*i);
  } else {
    const AlternateTaskVector::const_reverse_iterator end = task_points.rend();
    for (AlternateTaskVector::const_reverse_iterator i = task_points.rbegin();
         i != end; ++i)
      visitor.Visit((const TaskPoint &)*i);
  }
}

void
AbortTask::reset()
{
  clear();
  UnorderedTask::reset();
}

GeoVector 
AbortTask::get_vector_home(const AIRCRAFT_STATE &state) const
{
  const Waypoint *home_waypoint = waypoints.GetHome();
  if (home_waypoint)
    return GeoVector(state.Location, home_waypoint->Location);

  return GeoVector(fixed_zero);
}

GeoPoint 
AbortTask::get_task_center(const GeoPoint& fallback_location) const
{
  if (task_points.empty())
    return fallback_location;

  TaskProjection task_projection;
  for (unsigned i = 0; i < task_points.size(); ++i) {
    const GeoPoint location = task_points[i].get_location();
    if (i == 0)
      task_projection.reset(location);
    else
      task_projection.scan_location(location);
  }
  task_projection.update_fast();
  return task_projection.get_center();
}

fixed 
AbortTask::get_task_radius(const GeoPoint& fallback_location) const
{ 
  if (task_points.empty())
    return fixed_zero;

  TaskProjection task_projection;
  for (unsigned i = 0; i < task_points.size(); ++i) {
    const GeoPoint location = task_points[i].get_location();
    if (i == 0)
      task_projection.reset(location);
    else
      task_projection.scan_location(location);
  }
  task_projection.update_fast();
  return task_projection.ApproxRadius();
}

