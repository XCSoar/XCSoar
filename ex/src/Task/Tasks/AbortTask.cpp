/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

AbortTask::AbortTask(const TaskEvents &te, 
                     const TaskBehaviour &tb,
                     TaskAdvance &ta,
                     GlidePolar &gp,
                     const Waypoints &wps):
  UnorderedTask(te, tb, ta, gp), 
  active_waypoint(0),
  waypoints(wps),
  polar_safety(gp)
{
  // default MC value
  polar_safety.set_mc(1.0);
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
    active_waypoint = tps[index]->get_waypoint().id;
  }
}

TaskPoint* 
AbortTask::getActiveTaskPoint() const
{
  if (activeTaskPoint<tps.size()) {
    return tps[activeTaskPoint];
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
  for (std::vector< TaskPoint* >::iterator v=tps.begin();
       v != tps.end(); ) {
    delete (*v); 
    tps.erase(v);
  }
  m_landable_reachable = false;
}


double
AbortTask::abort_range(const AIRCRAFT_STATE &state)
{
  // always scan at least 50km or approx glide range
  return max(50000.0, state.NavAltitude*polar_safety.get_bestLD());
}

GlidePolar
AbortTask::get_safety_polar() const
{
  fixed mc_safety = polar_safety.get_mc();
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
  return (tps.size()>=10);
}


fixed 
AbortTask::is_reachable(const AIRCRAFT_STATE &state,
                        const Waypoint& waypoint,
                        const GlidePolar &polar,
                        const bool final_glide) const
{
  UnorderedTaskPoint t(waypoint, task_behaviour);
  GlideResult r = TaskSolution::glide_solution_remaining(t, state, polar);
  if (!r.glide_reachable(final_glide)) {
    return -fixed_one;
  } else {
    return r.TimeElapsed;
  }
}


bool
AbortTask::fill_reachable(const AIRCRAFT_STATE &state,
                          WaypointVector &approx_waypoints,
                          const GlidePolar &polar,
                          const bool only_airfield,
                          const bool final_glide)
{  
  if (task_full()) {
    return false;
  }
  bool found = false;
  std::priority_queue<WP_ALT, std::vector<WP_ALT>, Rank> q;
  for (WaypointVector::iterator v = approx_waypoints.begin();
       v!=approx_waypoints.end(); ) {

    if (only_airfield && !v->Flags.Airport) {
      continue;
    }
    const fixed t_elapsed = is_reachable(state, *v, polar, final_glide);
    if (!negative(t_elapsed)) {
      q.push(std::make_pair(*v, t_elapsed));
      // remove it since it's already in the list now      
      approx_waypoints.erase(v);

      found = true;

    } else {
      v++;
    }
  }
  while (!q.empty() && !task_full()) {
    tps.push_back(new UnorderedTaskPoint(q.top().first, task_behaviour));

    const int i = tps.size()-1;
    if (tps[i]->get_waypoint().id == active_waypoint) {
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
  WaypointVisitorVector(AbortTask::WaypointVector& wpv):vector(wpv) {};

/** 
 * Visit method, adds result to vector
 * 
 * @param wp Waypoint that is visited
 */
  void Visit(const Waypoint& wp) {
    vector.push_back(wp);
  }
private:
  AbortTask::WaypointVector &vector;
};


bool 
AbortTask::update_sample(const AIRCRAFT_STATE &state, 
                         const bool full_update)
{
  update_polar();
  clear();

  const unsigned active_waypoint_on_entry = active_waypoint;
  activeTaskPoint = 0; // default to best result if can't find user-set one 

  WaypointVector approx_waypoints; 

  WaypointVisitorVector wvv(approx_waypoints);
  waypoints.visit_within_radius(state.Location, abort_range(state), wvv);
  remove_unlandable(approx_waypoints);
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

  // now try with non-safety polar
  fill_reachable(state, approx_waypoints, glide_polar, true, false);
  fill_reachable(state, approx_waypoints, glide_polar, false, false);

  // now try with fake height added
  AIRCRAFT_STATE fake = state;
  fake.NavAltitude += 10000.0;
  fill_reachable(fake, approx_waypoints, glide_polar, true, false);
  fill_reachable(fake, approx_waypoints, glide_polar, false, false);

  if (tps.size()) {
    active_waypoint = tps[activeTaskPoint]->get_waypoint().id;
    if (active_waypoint_on_entry != active_waypoint) {
      task_events.active_changed(*tps[activeTaskPoint]);
    }
  }

  return false; // nothing to do
}



void 
AbortTask::update_offline(const AIRCRAFT_STATE &state)
{
  update_polar();
  m_landable_reachable = false;

  WaypointVector approx_waypoints; 

  WaypointVisitorVector wvv(approx_waypoints);
  waypoints.visit_within_radius(state.Location, abort_range(state), wvv);
  remove_unlandable(approx_waypoints);

  for (WaypointVector::iterator v = approx_waypoints.begin();
       v!=approx_waypoints.end(); ++v) {

    if (!negative(is_reachable(state, *v, polar_safety, true))) {
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
AbortTask::Accept(TaskPointVisitor& visitor, const bool reverse) const
{
  for (std::vector<TaskPoint*>::const_iterator 
         i= tps.begin(); i!= tps.end(); ++i) {

    (*i)->Accept(visitor);
  }
}

void 
AbortTask::remove_unlandable(WaypointVector &approx_waypoints)
{
  for (WaypointVector::iterator v = approx_waypoints.begin();
       v!=approx_waypoints.end(); ) {

    if (!v->is_landable()) {
      approx_waypoints.erase(v);
    } else {
      ++v;
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
    GeoVector null_vector(fixed_zero, fixed_zero);
    return null_vector;
  }
}
