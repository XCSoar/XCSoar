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

#include "OrderedTask.hpp"
#include "BaseTask/OrderedTaskPoint.hpp"
#include "PathSolvers/TaskDijkstra.hpp"
#include "TaskSolvers/TaskMacCreadyTravelled.hpp"
#include "TaskSolvers/TaskMacCreadyRemaining.hpp"
#include "TaskSolvers/TaskMacCreadyTotal.hpp"
#include "TaskSolvers/TaskCruiseEfficiency.hpp"
#include "TaskSolvers/TaskBestMc.hpp"
#include "TaskSolvers/TaskMinTarget.hpp"
#include "TaskSolvers/TaskGlideRequired.hpp"
#include "TaskSolvers/TaskOptTarget.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"
#include <algorithm>
#include <functional>

void
OrderedTask::update_geometry() 
{
  scan_start_finish();

  if (!has_start() || !tps[0]) {
    return;
  }

  for (unsigned i=0; i<tps.size(); i++) {
    if (i==0) {
      task_projection.reset(tps[i]->get_location());
    } else {
      task_projection.scan_location(tps[i]->get_location());
    }
  }
  task_projection.update_fast();

  std::for_each(tps.begin(), tps.end(), std::mem_fun(&OrderedTaskPoint::update_oz));

  if (has_start()) {
    // update stats so data can be used during task construction
    /// \todo this should only be done if not flying! (currently done with has_entered)
    if (!ts->has_entered()) {
      GEOPOINT loc = ts->get_location();
      update_stats_distances(loc, true);
    }
  }
}

////////// TIMES

double 
OrderedTask::scan_total_start_time(const AIRCRAFT_STATE &)
{
  if (ts) {
    return ts->get_state_entered().Time;
  } else {
    return 0.0;
  }
}

double 
OrderedTask::scan_leg_start_time(const AIRCRAFT_STATE &)
{
  if (activeTaskPoint) {
    return tps[activeTaskPoint-1]->get_state_entered().Time;
  } else {
    return -1;
  }
}

////////// DISTANCES

fixed
OrderedTask::scan_distance_min(const GEOPOINT &location, bool full)
{
  if (full) {
    SearchPoint ac(location, task_projection);
    TaskDijkstra dijkstra_min(*this);
    dijkstra_min.distance_min(ac);
    m_location_min_last = location;
  }
  return ts->scan_distance_min();
}

fixed
OrderedTask::scan_distance_max()
{
  // for max calculations, since one can still travel further in the
  // sector, we pretend we are on the previous turnpoint so the
  // search samples will contain the full boundary
  const unsigned atp = activeTaskPoint;
  if (atp) {
    activeTaskPoint--;
    ts->scan_active(tps[activeTaskPoint]);
  }
  TaskDijkstra dijkstra_max(*this);
  dijkstra_max.distance_max();

  if (atp) {
    activeTaskPoint = atp;
    ts->scan_active(tps[activeTaskPoint]);
  }
  return ts->scan_distance_max();
}

void
OrderedTask::scan_distance_minmax(const GEOPOINT &location, 
                                  bool force,
                                  double *dmin, double *dmax)
{
  if (!ts) {
    return;
  }

  if (force) {
    *dmax = scan_distance_max();
  }

  bool force_min = distance_is_significant(location, m_location_min_last) || force;
  *dmin = scan_distance_min(location, force_min);
}


fixed
OrderedTask::scan_distance_nominal()
{
  if (ts) {
    return ts->scan_distance_nominal();
  } else {
    return fixed_zero;
  }
}

fixed
OrderedTask::scan_distance_scored(const GEOPOINT &location)
{
  if (ts) {
    return ts->scan_distance_scored(location);
  } else {
    return fixed_zero;
  }
}

fixed
OrderedTask::scan_distance_remaining(const GEOPOINT &location)
{
  if (ts) {
    return ts->scan_distance_remaining(location);
  } else {
    return fixed_zero;
  }
}

fixed
OrderedTask::scan_distance_travelled(const GEOPOINT &location)
{
  if (ts) {
    return ts->scan_distance_travelled(location);
  } else {
    return fixed_zero;
  }
}

fixed
OrderedTask::scan_distance_planned()
{
  if (ts) {
    return ts->scan_distance_planned();
  } else {
    return fixed_zero;
  }
}

////////// TRANSITIONS

bool 
OrderedTask::check_transitions(const AIRCRAFT_STATE &state, 
                               const AIRCRAFT_STATE &state_last)
{
  if (!ts) {
    return false;
  }
  ts->scan_active(tps[activeTaskPoint]);

  const int n_task = tps.size();

  if (!n_task) {
    return false;
  }

  const int t_min = max(0,(int)activeTaskPoint-1);
  const int t_max = min(n_task-1, (int)activeTaskPoint+1);
  bool full_update = false;
  
  for (int i=t_min; i<=t_max; i++) {
    bool transition_enter = false;
    if (tps[i]->transition_enter(state, state_last)) {
      transition_enter = true;
      task_events.transition_enter(*tps[i]);
    }
    bool transition_exit = false;
    if (tps[i]->transition_exit(state, state_last)) {
      transition_exit = true;
      task_events.transition_exit(*tps[i]);
    }

    if ((i==(int)activeTaskPoint) && 
      task_advance.ready_to_advance(*tps[i],
                                    state,
                                    transition_enter,
                                    transition_exit)) {
      task_advance.set_armed(false);

      if (i+1<n_task) {

        i++;
        setActiveTaskPoint(i);
        ts->scan_active(tps[activeTaskPoint]);

        task_events.active_advanced(*tps[i],i);

        // on sector exit, must update samples since start sector
        // exit transition clears samples
        full_update = true;
      }
    }

    if (tps[i]->update_sample(state, task_events)) {
      full_update = true;
    }
  }

  ts->scan_active(tps[activeTaskPoint]);

  return full_update;
}

////////// ADDITIONAL FUNCTIONS

bool 
OrderedTask::update_idle(const AIRCRAFT_STATE& state)
{
  bool retval = AbstractTask::update_idle(state);

  if (has_start()
      && (task_behaviour.optimise_targets_range)
      && (task_behaviour.aat_min_time>0.0)) {

    if (activeTaskPoint>0) {
      double p = calc_min_target(state, task_behaviour.aat_min_time);
      (void)p;

      if (task_behaviour.optimise_targets_bearing) {
        if (AATPoint* ap = dynamic_cast<AATPoint*>(tps[activeTaskPoint])) {
          // very nasty hack
          TaskOptTarget tot(tps, activeTaskPoint, state, glide_polar,
                            *ap, ts);
          tot.search(0.5);
        }
      }
    }
    retval = true;
  }
  
  return retval;
}


bool 
OrderedTask::update_sample(const AIRCRAFT_STATE &state, 
                           const bool full_update)
{
  return true;
}

////////// TASK 

void
OrderedTask::set_neighbours(unsigned position)
{
  OrderedTaskPoint* prev=NULL;
  OrderedTaskPoint* next=NULL;

  if (!tps[position]) {
    // nothing to do if this is deleted
    return;
  }

  if (position>=tps.size()) {
    // nothing to do
    return;
  }

  if (position>0) {
    prev = tps[position-1];
  }
  if (position+1<tps.size()) {
    next = tps[position+1];
  }
  tps[position]->set_neighbours(prev, next);
}


bool
OrderedTask::check_task() const
{
  if (!tps.size()) {
    task_events.construction_error("Error! Empty task\n");
    return false;
  }
  if (!has_start()) {
    task_events.construction_error("Error! No start point\n");
    return false;
  }
  if (!has_finish()) {
    task_events.construction_error("Error! No finish point\n");
    return false;
  }
  return true;
}


bool 
OrderedTask::scan_start_finish()
{
  /// \todo also check there are not more than one start/finish point

  if (!tps.size()) {
    ts = NULL;
    tf = NULL;
    return false;
  }
  ts = dynamic_cast<StartPoint*>(tps[0]);
  if (tps.size()>1) {
    tf = dynamic_cast<FinishPoint*>(tps[tps.size()-1]);
  } else {
    tf = NULL;
  }
  return has_start() && has_finish();
}

void
OrderedTask::erase(const unsigned index)
{
  delete tps[index]; tps[index] = NULL;
  tps.erase(tps.begin()+index);
}

bool
OrderedTask::remove(const unsigned position)
{
  if (position>= tps.size()) {
    return false;
  }

  if (activeTaskPoint>position) {
    activeTaskPoint--;
  }

  erase(position);

  set_neighbours(position);
  if (position) {
    set_neighbours(position-1);
  }
  update_geometry();
  return true;
}


bool 
OrderedTask::append(OrderedTaskPoint* new_tp)
{
  tps.push_back(new_tp);
  if (tps.size()>1) {
    set_neighbours(tps.size()-2);
  }
  set_neighbours(tps.size()-1);
  update_geometry();
  return true;
}

bool 
OrderedTask::insert(OrderedTaskPoint* new_tp, 
                    const unsigned position)
{
  if (activeTaskPoint>=position) {
    activeTaskPoint++;
  }

  if (position<tps.size()) {
    tps.insert(tps.begin()+position, new_tp);
  } else {
    return append(new_tp);
  }
  if (position) {
    set_neighbours(position-1);
  }
  set_neighbours(position);
  set_neighbours(position+1);
  
  update_geometry();
  return true;
}

bool 
OrderedTask::replace(OrderedTaskPoint* new_tp, 
                     const unsigned position)
{
  if (position>=tps.size()) {
    return false;
  }
  if (tps[position]->equals(new_tp)) {
    // nothing to do
    return true;
  }

  delete tps[position];
  tps[position] = new_tp;

  if (position) {
    set_neighbours(position-1);
  }
  set_neighbours(position);
  if (position+1<tps.size()) {
    set_neighbours(position+1);
  }

  update_geometry();
  return true;
}


//////////  

void OrderedTask::setActiveTaskPoint(unsigned index)
{
  if (index<tps.size()) {
    activeTaskPoint = index;
  }
}

TaskPoint* OrderedTask::getActiveTaskPoint() const
{
  if (activeTaskPoint<tps.size()) {
    return tps[activeTaskPoint];
  } else {
    return NULL;
  }
}

////////// Glide functions

void
OrderedTask::glide_solution_remaining(const AIRCRAFT_STATE &aircraft, 
                                      GlideResult &total,
                                      GlideResult &leg)
{
  TaskMacCreadyRemaining tm(tps,activeTaskPoint,glide_polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution();
}

void
OrderedTask::glide_solution_travelled(const AIRCRAFT_STATE &aircraft, 
                                      GlideResult &total,
                                      GlideResult &leg)
{
  TaskMacCreadyTravelled tm(tps,activeTaskPoint,glide_polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution();
}

void
OrderedTask::glide_solution_planned(const AIRCRAFT_STATE &aircraft, 
                                    GlideResult &total,
                                    GlideResult &leg,
                                    DistanceRemainingStat &total_remaining_effective,
                                    DistanceRemainingStat &leg_remaining_effective,
                                    const double total_t_elapsed,
                                    const double leg_t_elapsed)
{
  TaskMacCreadyTotal tm(tps,activeTaskPoint,glide_polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution();

  total_remaining_effective.
    set_distance(tm.effective_distance(total_t_elapsed));

  leg_remaining_effective.
    set_distance(tm.effective_leg_distance(leg_t_elapsed));
}

////////// Auxiliary glide functions

double
OrderedTask::calc_glide_required(const AIRCRAFT_STATE &aircraft) 
{
  TaskGlideRequired bgr(tps, activeTaskPoint, aircraft, glide_polar);
  return bgr.search(0.0);
}

double
OrderedTask::calc_mc_best(const AIRCRAFT_STATE &aircraft)
{
  // note setting of lower limit on mc
  TaskBestMc bmc(tps,activeTaskPoint, aircraft, glide_polar);
  return bmc.search(glide_polar.get_mc());
}


double
OrderedTask::calc_cruise_efficiency(const AIRCRAFT_STATE &aircraft)
{
  if (activeTaskPoint>0) {
    TaskCruiseEfficiency bce(tps,activeTaskPoint, aircraft, glide_polar);
    return bce.search(1.0);
  } else {
    return 1.0;
  }
}

double
OrderedTask::calc_min_target(const AIRCRAFT_STATE &aircraft, 
                             const double t_target)
{
  if (stats.distance_max>stats.distance_min) {
    // only perform scan if modification is possible
    const double t_rem = max(0.0, t_target-stats.total.TimeElapsed);
    
    TaskMinTarget bmt(tps, activeTaskPoint, aircraft, glide_polar, t_rem, ts);
    double p= bmt.search(0.0);
    return p;
  } else {
    return 0.0;
  }
}


double 
OrderedTask::calc_gradient(const AIRCRAFT_STATE &state) 
{
  fixed g_best = fixed_zero;
  fixed d_acc = fixed_zero;
  fixed h_this = state.Altitude;

  for (unsigned i=activeTaskPoint; i< tps.size(); i++) {
    d_acc += tps[i]->get_vector_remaining(state).Distance;
    if (!d_acc) {
      continue;
    }
    const fixed g_this = (h_this-tps[i]->get_elevation())/d_acc;
    if (i==activeTaskPoint) {
      g_best = g_this;
    } else {
      g_best = min(g_best, g_this);
    }
  }
  return g_best;
}


////////// Constructors/destructors

OrderedTask::~OrderedTask()
{
  for (std::vector<OrderedTaskPoint*>::iterator v=tps.begin();
       v != tps.end(); ) {
    delete *v;
    tps.erase(v);
  }
}


OrderedTask::OrderedTask(const TaskEvents &te, 
                         const TaskBehaviour &tb,
                         TaskAdvance &ta,
                         GlidePolar &gp):
  AbstractTask(te, tb, ta, gp),
  ts(NULL),
  tf(NULL)
{
}

////////////////////////

void 
OrderedTask::Accept(TaskPointVisitor& visitor) const
{
  std::for_each(tps.begin(), tps.end(), 
                std::bind2nd(std::mem_fun(&OrderedTaskPoint::Accept), visitor));
}


void
OrderedTask::reset()
{
  /// \todo also reset data in this class e.g. stats?
  std::for_each(tps.begin(), tps.end(), std::mem_fun(&OrderedTaskPoint::reset));
}


const OrderedTaskPoint* 
OrderedTask::getTaskPoint(const unsigned index) const
{
  if (index>=tps.size()) {
    return NULL;
  } else {
    return tps[index];
  }
}


bool 
OrderedTask::has_start() const
{
  return (ts != NULL);
}

bool 
OrderedTask::has_finish() const
{
  return (tf != NULL);
}


bool 
OrderedTask::task_finished() const
{
  if (tf) {
    return (tf->has_entered());
  }
  return false;
}

bool 
OrderedTask::task_started() const
{
  if (ts) {
    return (ts->has_exited());
  }
  return false;
}


bool 
OrderedTask::distance_is_significant(const GEOPOINT &location,
                                     const GEOPOINT &location_last) const
{
  SearchPoint a1(location, task_projection);
  SearchPoint a2(location_last, task_projection);
  return TaskDijkstra::distance_is_significant(a1, a2);
}
