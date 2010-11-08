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
#include "AbortTask.hpp"
#include "Navigation/Aircraft.hpp"
#include "BaseTask/UnorderedTaskPoint.hpp"
#include <queue>
#include "Task/Visitors/TaskPointVisitor.hpp"
#include "TaskSolvers/TaskSolution.hpp"
#include "Task/TaskEvents.hpp"


static const unsigned AbortTask::max_abort = 10; 

AbortTask::AbortTask(TaskEvents &te, 
                     const TaskBehaviour &tb,
                     GlidePolar &gp,
                     const Waypoints &wps):
  UnorderedTask(ABORT, te, tb, gp),
  active_waypoint(0),
  waypoints(wps),
  polar_safety(gp)
{
}

AbortTask::~AbortTask()
{
  clear();
}

void 
AbortTask::setActiveTaskPoint(unsigned index)
{
  if (index<tps.size()) {
    activeTaskPoint = index;
    active_waypoint = tps[index].first->get_waypoint().id;
  }
}

TaskPoint* 
AbortTask::getActiveTaskPoint() const
{
  if (activeTaskPoint<tps.size()) {
    return tps[activeTaskPoint].first;
  } else {
    return NULL;
  }
}


bool 
AbortTask::validTaskPoint(const int index_offset) const
{
  unsigned index = activeTaskPoint+index_offset;
  return (index<tps.size());
}


unsigned
AbortTask::task_size() const
{
  return tps.size();
}

void 
AbortTask::clear() {
  for (AlternateTaskVector::iterator v=tps.begin();
       v != tps.end(); ) {
    delete (v->first); 
    tps.erase(v);
  }
  m_landable_reachable = false;
}

fixed
AbortTask::abort_range(const AIRCRAFT_STATE &state) const
{
  // always scan at least 50km or approx glide range
  return max(fixed(50000.0), state.NavAltitude*polar_safety.get_bestLD());
}

GlidePolar
AbortTask::get_safety_polar() const
{
  const fixed mc_safety = task_behaviour.get_safety_mc(glide_polar.get_mc());
  GlidePolar polar = glide_polar;
  polar.set_mc(mc_safety);
  return polar;
}

void
AbortTask::update_polar()
{
  // glide_polar for task
  polar_safety = get_safety_polar();
}

bool
AbortTask::task_full() const
{
  return (tps.size()>= max_abort);
}

/**
 * Function object used to rank waypoints by arrival time
 */
struct AbortRank : public std::binary_function<AbortTask::Alternate, 
                                                   AbortTask::Alternate, bool> {
  /**
   * Condition, ranks by arrival time 
   */
  bool operator()(const AbortTask::Alternate& x, 
                  const AbortTask::Alternate& y) const {
    return x.second.TimeElapsed > y.second.TimeElapsed;
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
                          const bool final_glide)
{
  if (task_full()) {
    return false;
  }
  bool found = false;
  std::priority_queue<Alternate, AlternateVector, AbortRank> q;
  for (AlternateVector::iterator v = approx_waypoints.begin();
       v!=approx_waypoints.end(); ) {

    if (only_airfield && !v->first.Flags.Airport) {
      ++v;
      continue;
    }
    UnorderedTaskPoint t(v->first, task_behaviour);
    const GlideResult result = TaskSolution::glide_solution_remaining(t, state, polar);
    if (is_reachable(result, final_glide)) {
      q.push(std::make_pair(v->first, result));
      // remove it since it's already in the list now      
      approx_waypoints.erase(v);

      found = true;

    } else {
      ++v;
    }
  }
  while (!q.empty() && !task_full()) {
    const Alternate top = q.top();
    tps.push_back(std::make_pair(new UnorderedTaskPoint(top.first, task_behaviour),
                                 top.second));

    const int i = tps.size()-1;
    if (tps[i].first->get_waypoint().id == active_waypoint) {
      activeTaskPoint = i;
    }

    q.pop();
  }
  return found;
}


#include "Waypoint/WaypointVisitor.hpp"

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
    if (wp.is_landable()) {
      vector.push_back(std::make_pair(wp, GlideResult()));
    }
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
  update_polar();
  clear();

  const unsigned active_waypoint_on_entry = active_waypoint;
  activeTaskPoint = 0; // default to best result if can't find user-set one 

  AlternateVector approx_waypoints; 

  WaypointVisitorVector wvv(approx_waypoints);
  waypoints.visit_within_radius(state.Location, abort_range(state), wvv);
  if (!approx_waypoints.size()) {
    /**
     * \todo
     * - increase range
     */
    return false;
  }

  // sort by alt difference

  // first try with safety polar, final glide only
  m_landable_reachable|=  fill_reachable(state, approx_waypoints, polar_safety, true, true);
  m_landable_reachable|=  fill_reachable(state, approx_waypoints, polar_safety, false, true);

  // inform clients that the landable reachable scan has been performed 
  client_update(state, true);

  // now try with non-safety polar
  fill_reachable(state, approx_waypoints, glide_polar, true, false);
  fill_reachable(state, approx_waypoints, glide_polar, false, false);

  // now try with fake height added
  AIRCRAFT_STATE fake = state;
  fake.NavAltitude += fixed(10000.0);
  fill_reachable(fake, approx_waypoints, glide_polar, true, false);
  fill_reachable(fake, approx_waypoints, glide_polar, false, false);

  // inform clients that the landable unreachable scan has been performed 
  client_update(state, false);

  if (tps.size()) {
    active_waypoint = tps[activeTaskPoint].first->get_waypoint().id;
    if (is_active && (active_waypoint_on_entry != active_waypoint)) {
      task_events.active_changed(*(tps[activeTaskPoint].first));
    }
  }

  return false; // nothing to do
}



void 
AbortTask::update_offline(const AIRCRAFT_STATE &state)
{
  update_polar();
  m_landable_reachable = false;

  AlternateVector approx_waypoints; 

  WaypointVisitorVector wvv(approx_waypoints);
  waypoints.visit_within_radius(state.Location, abort_range(state), wvv);

  for (AlternateVector::iterator v = approx_waypoints.begin();
       v!=approx_waypoints.end(); ++v) {

    UnorderedTaskPoint t(v->first, task_behaviour);
    const GlideResult result = TaskSolution::glide_solution_remaining(t, state, polar_safety);
    if (is_reachable(result, true)) {
      m_landable_reachable = true;
      return;
    }
  }
}

bool 
AbortTask::check_transitions(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&)
{
  return false; // nothing to do
}

void 
AbortTask::tp_CAccept(TaskPointConstVisitor& visitor, const bool reverse) const
{
  if (!reverse) {
    for (AlternateTaskVector::const_iterator 
           i= tps.begin(); i!= tps.end(); ++i) {
      visitor.Visit(*i->first);
    }
  } else {
    for (AlternateTaskVector::const_reverse_iterator 
           i= tps.rbegin(); i!= tps.rend(); ++i) {
      visitor.Visit(*i->first);
    }
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
  const Waypoint* home_waypoint = waypoints.find_home();
  if (home_waypoint) {
    return GeoVector(state.Location, home_waypoint->Location);
  } else {
    GeoVector null_vector(fixed_zero);
    return null_vector;
  }
}

GeoPoint 
AbortTask::get_task_center(const GeoPoint& fallback_location) const
{
  if (tps.empty()) {
    return fallback_location;
  } else {
    TaskProjection task_projection;
    for (unsigned i=0; i<tps.size(); ++i) {
      if (i==0) {
        task_projection.reset(tps[i].first->get_location());
      }
      task_projection.scan_location(tps[i].first->get_location());
    }
    task_projection.update_fast();
    return task_projection.get_center();
  }
}

fixed 
AbortTask::get_task_radius(const GeoPoint& fallback_location) const
{ 
  if (tps.empty()) {
    return fixed_zero;
  } else {
    TaskProjection task_projection;
    for (unsigned i=0; i<tps.size(); ++i) {
      if (i==0) {
        task_projection.reset(tps[i].first->get_location());
      }
      task_projection.scan_location(tps[i].first->get_location());
    }
    task_projection.update_fast();
    return task_projection.get_radius();
  }
}

