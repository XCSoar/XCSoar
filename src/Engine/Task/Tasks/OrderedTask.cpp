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
#include "Task/TaskEvents.hpp"
#include "Task/TaskAdvance.hpp"
#include "BaseTask/OrderedTaskPoint.hpp"
#include "PathSolvers/TaskDijkstra.hpp"
#include "TaskSolvers/TaskMacCreadyTravelled.hpp"
#include "TaskSolvers/TaskMacCreadyRemaining.hpp"
#include "TaskSolvers/TaskMacCreadyTotal.hpp"
#include "TaskSolvers/TaskCruiseEfficiency.hpp"
#include "TaskSolvers/TaskEffectiveMacCready.hpp"
#include "TaskSolvers/TaskBestMc.hpp"
#include "TaskSolvers/TaskMinTarget.hpp"
#include "TaskSolvers/TaskGlideRequired.hpp"
#include "TaskSolvers/TaskOptTarget.hpp"
#include "Task/Visitors/TaskPointVisitor.hpp"

#include "Task/Factory/RTTaskFactory.hpp"
#include "Task/Factory/FAITaskFactory.hpp"
#include "Task/Factory/FAITriangleTaskFactory.hpp"
#include "Task/Factory/FAIORTaskFactory.hpp"
#include "Task/Factory/FAIGoalTaskFactory.hpp"
#include "Task/Factory/AATTaskFactory.hpp"
#include "Task/Factory/MixedTaskFactory.hpp"
#include "Task/Factory/TouringTaskFactory.hpp"

#include "Waypoint/Waypoints.hpp"

void
OrderedTask::update_geometry() 
{
  scan_start_finish();

  if (!has_start() || !tps[0])
    return;

  for (unsigned i = 0; i < tps.size(); ++i) {
    if (i == 0)
      task_projection.reset(tps[i]->get_location());

    tps[i]->scan_projection(task_projection);
  }
  task_projection.update_fast();

  for (OrderedTaskPointVector::iterator it = tps.begin(); it!= tps.end(); it++)
    (*it)->update_oz();

  if (has_start()) {
    // update stats so data can be used during task construction
    /// @todo this should only be done if not flying! (currently done with has_entered)
    if (!ts->has_entered()) {
      GeoPoint loc = ts->get_location();
      update_stats_distances(loc, true);
      if (has_finish()) {
        /// @todo: call AbstractTask::update stats methods with fake state
        /// so stats are updated
      }
    }
  }
}

// TIMES

fixed 
OrderedTask::scan_total_start_time(const AIRCRAFT_STATE &)
{
  if (ts)
    return ts->get_state_entered().Time;

  return fixed_zero;
}

fixed 
OrderedTask::scan_leg_start_time(const AIRCRAFT_STATE &)
{
  if (activeTaskPoint)
    return tps[activeTaskPoint-1]->get_state_entered().Time;

  return -fixed_one;
}

// DISTANCES

fixed
OrderedTask::scan_distance_min(const GeoPoint &location, bool full)
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
OrderedTask::scan_distance_minmax(const GeoPoint &location, 
                                  bool force,
                                  fixed *dmin, fixed *dmax)
{
  if (!ts)
    return;

  if (force)
    *dmax = scan_distance_max();

  bool force_min = distance_is_significant(location, m_location_min_last)
                   || force;
  *dmin = scan_distance_min(location, force_min);
}

fixed
OrderedTask::scan_distance_nominal()
{
  if (ts)
    return ts->scan_distance_nominal();

  return fixed_zero;
}

fixed
OrderedTask::scan_distance_scored(const GeoPoint &location)
{
  if (ts)
    return ts->scan_distance_scored(location);

  return fixed_zero;
}

fixed
OrderedTask::scan_distance_remaining(const GeoPoint &location)
{
  if (ts)
    return ts->scan_distance_remaining(location);

  return fixed_zero;
}

fixed
OrderedTask::scan_distance_travelled(const GeoPoint &location)
{
  if (ts)
    return ts->scan_distance_travelled(location);

  return fixed_zero;
}

fixed
OrderedTask::scan_distance_planned()
{
  if (ts)
    return ts->scan_distance_planned();

  return fixed_zero;
}

// TRANSITIONS

bool 
OrderedTask::check_transitions(const AIRCRAFT_STATE &state, 
                               const AIRCRAFT_STATE &state_last)
{
  if (!ts)
    return false;

  ts->scan_active(tps[activeTaskPoint]);

  if (!state.Flying)
    return false;

  const int n_task = tps.size();

  if (!n_task)
    return false;

  bool last_started = task_started();
  const bool last_finished = task_finished();

  const int t_min = max(0, (int)activeTaskPoint - 1);
  const int t_max = min(n_task - 1, (int)activeTaskPoint + 1);
  bool full_update = false;

  for (int i = t_min; i <= t_max; i++) {
    bool transition_enter = false;
    if (tps[i]->transition_enter(state, state_last)) {
      transition_enter = true;
      task_events.transition_enter(*tps[i]);
    }

    bool transition_exit = false;
    if (tps[i]->transition_exit(state, state_last)) {
      transition_exit = true;
      task_events.transition_exit(*tps[i]);
      
      // detect restart
      if ((i == 0) && last_started)
        last_started = false;
    }

    if ((activeTaskPoint == 0) && (i == 0))
      update_start_transition(state);

    if (tps[i]->update_sample(state, task_events))
      full_update = true;

    if (i == (int)activeTaskPoint) {
      const bool last_request_armed = task_advance.request_armed();

      if (task_advance.ready_to_advance(*tps[i], state,
                                        transition_enter,
                                        transition_exit)) {
        task_advance.set_armed(false);
        
        if (i + 1 < n_task) {
          i++;
          setActiveTaskPoint(i);
          ts->scan_active(tps[activeTaskPoint]);
          
          task_events.active_advanced(*tps[i], i);

          // on sector exit, must update samples since start sector
          // exit transition clears samples
          full_update = true;
        }
      } else if (!last_request_armed && task_advance.request_armed()) {
        task_events.request_arm(*tps[i]);
      }
    }
  }

  ts->scan_active(tps[activeTaskPoint]);

  stats.task_finished = task_finished();
  stats.task_started = task_started();

  if (stats.task_started)
    tf->set_fai_finish_height(get_start_state().NavAltitude - fixed(1000));

  if (stats.task_started && !last_started)
    task_events.task_start();

  if (stats.task_finished && !last_finished)
    task_events.task_finish();

  return full_update;
}

// ADDITIONAL FUNCTIONS

bool 
OrderedTask::update_idle(const AIRCRAFT_STATE& state)
{
  bool retval = AbstractTask::update_idle(state);

  if (has_start()
      && (task_behaviour.optimise_targets_range)
      && (get_ordered_task_behaviour().aat_min_time > fixed_zero)) {

    fixed p = calc_min_target(state, get_ordered_task_behaviour().aat_min_time + fixed(300));
    (void)p;

    if (task_behaviour.optimise_targets_bearing) {
      if (tps[activeTaskPoint]->type == TaskPoint::AAT) {
        AATPoint *ap = (AATPoint *)tps[activeTaskPoint];
        // very nasty hack
        TaskOptTarget tot(tps, activeTaskPoint, state, glide_polar, *ap, ts);
        tot.search(fixed(0.5));
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

// TASK

void
OrderedTask::set_neighbours(unsigned position)
{
  OrderedTaskPoint* prev = NULL;
  OrderedTaskPoint* next = NULL;

  if (!tps[position])
    // nothing to do if this is deleted
    return;

  if (position >= tps.size())
    // nothing to do
    return;

  if (position > 0)
    prev = tps[position - 1];

  if (position + 1 < tps.size())
    next = tps[position + 1];

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

OrderedTaskPoint*
OrderedTask::get_ordered_task_point(unsigned TPindex) const
{
 if (TPindex > tps.size() - 1) {
   return NULL;
 }
 return tps[TPindex];
}

AATPoint*
OrderedTask::get_AAT_task_point(unsigned TPindex) const
{
 if (TPindex > tps.size() - 1) {
   return NULL;
 }
 if (tps[TPindex]) {
    if (tps[TPindex]->type == TaskPoint::AAT)
      return (AATPoint*) tps[TPindex];
    else
      return (AATPoint*)NULL;
 }
 return NULL;
}

bool
OrderedTask::scan_start_finish()
{
  /// @todo also check there are not more than one start/finish point
  if (!tps.size()) {
    ts = NULL;
    tf = NULL;
    return false;
  }

  ts = tps[0]->type == TaskPoint::START
    ? (StartPoint *)tps[0]
    : NULL;

  tf = tps.size() > 1 && tps[tps.size() - 1]->type == TaskPoint::FINISH
    ? (FinishPoint *)tps[tps.size() - 1]
    : NULL;

  return has_start() && has_finish();
}

void
OrderedTask::erase(const unsigned index)
{
  delete tps[index];
  tps[index] = NULL;
  tps.erase(tps.begin() + index);
}

bool
OrderedTask::remove(const unsigned position)
{
  if (position >= tps.size())
    return false;

  if (activeTaskPoint > position)
    activeTaskPoint--;

  erase(position);

  set_neighbours(position);
  if (position)
    set_neighbours(position - 1);

  update_geometry();
  return true;
}

bool 
OrderedTask::append(OrderedTaskPoint* new_tp)
{
  tps.push_back(new_tp);
  if (tps.size() > 1)
    set_neighbours(tps.size() - 2);

  set_neighbours(tps.size() - 1);
  update_geometry();
  return true;
}

bool 
OrderedTask::insert(OrderedTaskPoint* new_tp, 
                    const unsigned position)
{
  if (activeTaskPoint >= position)
    activeTaskPoint++;

  if (position < tps.size())
    tps.insert(tps.begin() + position, new_tp);
  else
    return append(new_tp);

  if (position)
    set_neighbours(position - 1);

  set_neighbours(position);
  set_neighbours(position + 1);

  update_geometry();
  return true;
}

bool 
OrderedTask::replace(OrderedTaskPoint* new_tp, 
                     const unsigned position)
{
  if (position >= tps.size())
    return false;

  if (tps[position]->equals(new_tp))
    // nothing to do
    return true;

  delete tps[position];
  tps[position] = new_tp;

  if (position)
    set_neighbours(position - 1);

  set_neighbours(position);
  if (position + 1 < tps.size())
    set_neighbours(position + 1);

  update_geometry();
  return true;
}

void 
OrderedTask::setActiveTaskPoint(unsigned index)
{
  if (index < tps.size()) {
    if (activeTaskPoint != index)
      task_advance.set_armed(false);

    activeTaskPoint = index;
  } else if (tps.empty()) {
    activeTaskPoint = 0;
  }
}

TaskPoint* 
OrderedTask::getActiveTaskPoint() const
{
  if (activeTaskPoint < tps.size())
    return tps[activeTaskPoint];

  return NULL;
}

bool 
OrderedTask::validTaskPoint(const int index_offset) const
{
  unsigned index = activeTaskPoint + index_offset;
  return (index < tps.size());
}


void
OrderedTask::glide_solution_remaining(const AIRCRAFT_STATE &aircraft,
                                      const GlidePolar &polar,
                                      GlideResult &total,
                                      GlideResult &leg)
{
  TaskMacCreadyRemaining tm(tps, activeTaskPoint, polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution(aircraft);
  if (activeTaskPoint == 0)
    leg.Vector = GeoVector(aircraft.Location, ts->get_location_remaining());
}

void
OrderedTask::glide_solution_travelled(const AIRCRAFT_STATE &aircraft, 
                                      GlideResult &total,
                                      GlideResult &leg)
{
  TaskMacCreadyTravelled tm(tps, activeTaskPoint, glide_polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution(aircraft);
}

void
OrderedTask::glide_solution_planned(const AIRCRAFT_STATE &aircraft, 
                                    GlideResult &total,
                                    GlideResult &leg,
                                    DistanceStat &total_remaining_effective,
                                    DistanceStat &leg_remaining_effective,
                                    const fixed total_t_elapsed,
                                    const fixed leg_t_elapsed)
{
  TaskMacCreadyTotal tm(tps, activeTaskPoint, glide_polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution(aircraft);

  total_remaining_effective.set_distance(
      tm.effective_distance(fixed(total_t_elapsed)));

  leg_remaining_effective.set_distance(
      tm.effective_leg_distance(fixed(leg_t_elapsed)));
}

// Auxiliary glide functions

fixed
OrderedTask::calc_glide_required(const AIRCRAFT_STATE &aircraft) const
{
  TaskGlideRequired bgr(tps, activeTaskPoint, aircraft, glide_polar);
  return bgr.search(fixed_zero);
}

fixed
OrderedTask::calc_mc_best(const AIRCRAFT_STATE &aircraft) const
{
  // note setting of lower limit on mc
  TaskBestMc bmc(tps,activeTaskPoint, aircraft, glide_polar);
  return bmc.search(glide_polar.get_mc());
}


fixed
OrderedTask::calc_cruise_efficiency(const AIRCRAFT_STATE &aircraft) const
{
  if (activeTaskPoint > 0) {
    TaskCruiseEfficiency bce(tps, activeTaskPoint, aircraft, glide_polar);
    return bce.search(fixed_one);
  }

  return fixed_one;
}

fixed 
OrderedTask::calc_effective_mc(const AIRCRAFT_STATE &aircraft) const
{
  if (activeTaskPoint > 0) {
    TaskEffectiveMacCready bce(tps,activeTaskPoint, aircraft, glide_polar);
    return bce.search(glide_polar.get_mc());
  }

  return glide_polar.get_mc();
}


fixed
OrderedTask::calc_min_target(const AIRCRAFT_STATE &aircraft, 
                             const fixed t_target) const
{
  if (stats.distance_max > stats.distance_min) {
    // only perform scan if modification is possible
    const fixed t_rem = max(fixed_zero, t_target - stats.total.TimeElapsed);

    TaskMinTarget bmt(tps, activeTaskPoint, aircraft, glide_polar, t_rem, ts);
    fixed p = bmt.search(fixed_zero);
    return p;
  }

  return fixed_zero;
}


fixed 
OrderedTask::calc_gradient(const AIRCRAFT_STATE &state) const
{
  if (tps.size() < 1)
    return fixed_zero;

  // Iterate through remaining turnpoints
  fixed distance = fixed_zero;
  for (unsigned i = activeTaskPoint; i < tps.size(); i++)
    // Sum up the leg distances
    distance += tps[i]->get_vector_remaining(state).Distance;

  if (!distance)
    return fixed_zero;

  // Calculate gradient to the last turnpoint of the remaining task
  return (state.NavAltitude - tps[tps.size() - 1]->get_elevation()) / distance;
}

// Constructors/destructors

OrderedTask::~OrderedTask()
{
  for (OrderedTaskPointVector::iterator v = tps.begin(); v != tps.end();) {
    delete *v;
    tps.erase(v);
  }
  delete active_factory;
}

OrderedTask::OrderedTask(TaskEvents &te, 
                         const TaskBehaviour &tb,
                         GlidePolar &gp):
  AbstractTask(ORDERED, te, tb, gp),
  ts(NULL),
  tf(NULL),
  factory_mode(FACTORY_FAI_GENERAL),
  active_factory(NULL),
  m_ordered_behaviour(tb.ordered_defaults),
  task_advance(m_ordered_behaviour)
{
  active_factory = new FAITaskFactory(*this, task_behaviour);
  active_factory->update_ordered_task_behaviour(m_ordered_behaviour);
}

void 
OrderedTask::tp_CAccept(TaskPointConstVisitor& visitor, const bool reverse) const
{
  if (!reverse) {
    for (OrderedTaskPointVector::const_iterator it = tps.begin(); 
         it!= tps.end(); ++it)
      visitor.Visit(**it);
  } else {
    for (OrderedTaskPointVector::const_reverse_iterator it = tps.rbegin(); 
         it!= tps.rend(); ++it)
      visitor.Visit(**it);
  }
}

void 
OrderedTask::tp_Accept(TaskPointVisitor& visitor, const bool reverse)
{
  if (!reverse) {
    for (OrderedTaskPointVector::iterator it = tps.begin(); 
         it!= tps.end(); ++it)
      visitor.Visit(**it);
  } else {
    for (OrderedTaskPointVector::reverse_iterator it = tps.rbegin(); 
         it!= tps.rend(); ++it)
      visitor.Visit(**it);
  }
}

void
OrderedTask::reset()
{  
  /// @todo also reset data in this class e.g. stats?
  for (OrderedTaskPointVector::iterator it = tps.begin(); it!= tps.end(); it++)
    (*it)->reset();

  AbstractTask::reset();
  stats.task_finished = false;
  stats.task_started = false;
  task_advance.reset();
  setActiveTaskPoint(0);
}

const OrderedTaskPoint* 
OrderedTask::getTaskPoint(const unsigned index) const
{
  if (index >= tps.size())
    return NULL;

  return tps[index];
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
  if (tf)
    return (tf->has_entered());

  return false;
}

bool 
OrderedTask::task_started() const
{
  if (ts)
    return (ts->has_exited());

  return false;
}


bool 
OrderedTask::distance_is_significant(const GeoPoint &location,
                                     const GeoPoint &location_last) const
{
  SearchPoint a1(location, task_projection);
  SearchPoint a2(location_last, task_projection);
  return TaskDijkstra::distance_is_significant(a1, a2);
}


const SearchPointVector& 
OrderedTask::get_tp_search_points(unsigned tp) const 
{
  return tps[tp]->get_search_points();
}

void 
OrderedTask::set_tp_search_min(unsigned tp, const SearchPoint &sol) 
{
  if (!tp && !tps[0]->has_exited()) 
    return;

  tps[tp]->set_search_min(sol);
}

void 
OrderedTask::set_tp_search_achieved(unsigned tp, const SearchPoint &sol) 
{
  if (tps[tp]->has_sampled())
    set_tp_search_min(tp, sol);
}

void 
OrderedTask::set_tp_search_max(unsigned tp, const SearchPoint &sol) 
{
  tps[tp]->set_search_max(sol);
}

unsigned 
OrderedTask::task_size() const 
{
  return tps.size();
}

unsigned 
OrderedTask::getActiveIndex() const 
{
  return activeTaskPoint;
}

TaskProjection& 
OrderedTask::get_task_projection() 
{
  return task_projection;
}

void
OrderedTask::update_start_transition(const AIRCRAFT_STATE &state)
{
  if (ts->has_exited())
    return;

  if (ts->isInSector(state)) {
    // @todo find boundary point that produces shortest
    // distance from state to that point to next tp point
    ts->find_best_start(state, *tps[1]);
  } else {
    // reset on invalid transition to outside
    // point to nominal start point
    ts->reset();
  }
}

AIRCRAFT_STATE 
OrderedTask::get_start_state() const
{
  if (has_start() && task_started()) 
    return ts->get_state_entered();

  AIRCRAFT_STATE null_state;
  return null_state;
}

AIRCRAFT_STATE 
OrderedTask::get_finish_state() const
{
  if (has_finish() && task_finished()) 
    return tf->get_state_entered();

  AIRCRAFT_STATE null_state;
  return null_state;
}

bool
OrderedTask::has_targets() const
{
  for (unsigned i = 0; i < tps.size(); ++i)
    if (tps[i]->has_target())
      return true;

  return false;
}

fixed
OrderedTask::get_finish_height() const
{
  if (tf)
    return tf->get_elevation();

  return fixed_zero;
}

GeoPoint 
OrderedTask::get_task_center(const GeoPoint& fallback_location) const
{
  if (!has_start() || !tps[0])
    return fallback_location;

  return task_projection.get_center();
}

fixed 
OrderedTask::get_task_radius(const GeoPoint& fallback_location) const
{ 
  if (!has_start() || !tps[0])
    return fixed_zero;

  return task_projection.get_radius();
}

OrderedTask* 
OrderedTask::clone(TaskEvents &te, 
                   const TaskBehaviour &tb,
                   GlidePolar &gp) const
{
  OrderedTask* new_task = new OrderedTask(te, tb, gp);

  new_task->activeTaskPoint = activeTaskPoint;
  new_task->m_ordered_behaviour = m_ordered_behaviour;

  new_task->set_factory(factory_mode);
  for (unsigned i = 0; i < tps.size(); ++i) {
    new_task->append(tps[i]->clone(tb, new_task->m_ordered_behaviour,
                                   new_task->get_task_projection()));
  }
  new_task->update_geometry();
  return new_task;
}

bool
OrderedTask::check_duplicate_waypoints(Waypoints& waypoints)
{
  bool changed = false;
  for (unsigned i = 0; i < task_size(); ++i) {
    Waypoint wp(tps[i]->get_waypoint());
    bool this_changed = !waypoints.find_duplicate(wp);
    changed |= this_changed;
    replace(tps[i]->clone(task_behaviour, m_ordered_behaviour,
                          get_task_projection(), &wp), i);
  }

  if (changed)
    waypoints.optimise();

  return changed;
}

bool
OrderedTask::commit(const OrderedTask& that)
{
  bool modified = false;

  // change mode to that one 
  set_factory(that.factory_mode);

  // copy across behaviour
  m_ordered_behaviour = that.m_ordered_behaviour;

  // remove if that task is smaller than this one
  while (task_size() > that.task_size()) {
    remove(task_size() - 1);
    modified = true;
  }

  for (unsigned i = 0; i < that.task_size(); ++i) {
    if (i >= task_size()) {
      // that task is larger than this
      append(that.tps[i]->clone(task_behaviour, m_ordered_behaviour,
                                get_task_projection()));
      modified = true;
    } else if (!tps[i]->equals(that.tps[i])) {
      // that task point is changed
      replace(that.tps[i]->clone(task_behaviour, m_ordered_behaviour,
                                 get_task_projection()), i);
      modified = true;
    }
  }

  if (modified)
    update_geometry();
    // @todo also re-scan task sample state,
    // potentially resetting task

  return modified;
}


bool
OrderedTask::relocate(const unsigned position, const Waypoint& waypoint) 
{
  if (position >= task_size())
    return false;

  OrderedTaskPoint *new_tp = tps[position]->clone(task_behaviour,
                                                  m_ordered_behaviour,
                                                  task_projection,
                                                  &waypoint);
  return replace(new_tp, position);
}

OrderedTask::Factory_t 
OrderedTask::set_factory(const Factory_t the_factory)
{
  // detect no change
  if (factory_mode == the_factory)
    return factory_mode;

  if (the_factory != FACTORY_MIXED) {
    // can switch from anything to mixed, otherwise need reset
    reset();

    /// @todo call into task_events to ask if reset is desired on
    /// factory change
  }
  factory_mode = the_factory;

  delete active_factory;

  switch (factory_mode) {
  case FACTORY_RT:
    active_factory = new RTTaskFactory(*this, task_behaviour);
    break;
  case FACTORY_FAI_GENERAL:
    active_factory = new FAITaskFactory(*this, task_behaviour);
    break;
  case FACTORY_FAI_TRIANGLE:
    active_factory = new FAITriangleTaskFactory(*this, task_behaviour);
    break;
  case FACTORY_FAI_OR:
    active_factory = new FAIORTaskFactory(*this, task_behaviour);
    break;
  case FACTORY_FAI_GOAL:
    active_factory = new FAIGoalTaskFactory(*this, task_behaviour);
    break;
  case FACTORY_AAT:
    active_factory = new AATTaskFactory(*this, task_behaviour);
    break;
  case FACTORY_MIXED:
    active_factory = new MixedTaskFactory(*this, task_behaviour);
    break;
  case FACTORY_TOURING:
    active_factory = new TouringTaskFactory(*this, task_behaviour);
    break;
  };
  active_factory->update_ordered_task_behaviour(m_ordered_behaviour);

  return factory_mode;
}

void 
OrderedTask::set_ordered_task_behaviour(const OrderedTaskBehaviour& ob)
{
  m_ordered_behaviour = ob;
}

bool 
OrderedTask::is_scored() const
{
  return m_ordered_behaviour.task_scored;
}

std::vector<OrderedTask::Factory_t> 
OrderedTask::get_factory_types(bool all) const
{
  /// @todo: check transform types if all=false
  std::vector<Factory_t> f_list;
  f_list.push_back(FACTORY_FAI_GENERAL);
  f_list.push_back(FACTORY_FAI_TRIANGLE);
  f_list.push_back(FACTORY_FAI_OR);
  f_list.push_back(FACTORY_FAI_GOAL);
  f_list.push_back(FACTORY_RT);
  f_list.push_back(FACTORY_AAT);
  f_list.push_back(FACTORY_MIXED);
  f_list.push_back(FACTORY_TOURING);
  return f_list;
}

void 
OrderedTask::clear()
{
  while (tps.size())
    erase(0);

  reset();
  m_ordered_behaviour = task_behaviour.ordered_defaults;
}

OrderedTaskPoint* 
OrderedTask::get_tp(const unsigned position)
{
  if (position >= task_size())
    return NULL;

  return tps[position];
}

const OrderedTaskPoint* 
OrderedTask::get_tp(const unsigned position) const
{
  if (position >= task_size())
    return NULL;

  return tps[position];
}
