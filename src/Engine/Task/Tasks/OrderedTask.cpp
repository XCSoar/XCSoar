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

#include "OrderedTask.hpp"
#include "Task/TaskEvents.hpp"
#include "Task/TaskAdvance.hpp"
#include "BaseTask/OrderedTaskPoint.hpp"
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

#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Geo/GeoBounds.hpp"
#include "Task/TaskStats/TaskSummary.hpp"

static void
SetTaskBehaviour(OrderedTask::OrderedTaskPointVector &vector,
                 const TaskBehaviour &tb)
{
  OrderedTask::OrderedTaskPointVector::iterator end = vector.end();
  for (OrderedTask::OrderedTaskPointVector::iterator i = vector.begin();
       i != end; ++i)
    (*i)->SetTaskBehaviour(tb);
}

void
OrderedTask::SetTaskBehaviour(const TaskBehaviour &tb)
{
  AbstractTask::SetTaskBehaviour(tb);

  ::SetTaskBehaviour(task_points, tb);
  ::SetTaskBehaviour(optional_start_points, tb);
}

static void
UpdateObservationZones(OrderedTask::OrderedTaskPointVector &points,
                       const TaskProjection &task_projection)
{
  OrderedTask::OrderedTaskPointVector::iterator end = points.end();
  for (OrderedTask::OrderedTaskPointVector::iterator i = points.begin();
       i != end; ++i)
    (*i)->update_oz(task_projection);
}

void
OrderedTask::update_geometry() 
{
  scan_start_finish();

  if (!has_start() || !task_points[0])
    return;

  // scan location of task points
  for (unsigned i = 0; i < task_points.size(); ++i) {
    if (i == 0)
      task_projection.reset(task_points[i]->get_location());

    task_points[i]->scan_projection(task_projection);
  }
  // ... and optional start points
  for (unsigned i = 0; i < optional_start_points.size(); ++i) {
    optional_start_points[i]->scan_projection(task_projection);
  }

  // projection can now be determined
  task_projection.update_fast();

  // update OZ's for items that depend on next-point geometry 
  UpdateObservationZones(task_points, task_projection);
  UpdateObservationZones(optional_start_points, task_projection);

  // now that the task projection is stable, and oz is stable,
  // calculate the bounding box in projected coordinates
  for (unsigned i = 0; i < task_points.size(); ++i) {
    task_points[i]->update_boundingbox(task_projection);
  }
  for (unsigned i = 0; i < optional_start_points.size(); ++i) {
    optional_start_points[i]->update_boundingbox(task_projection);
  }

  // update stats so data can be used during task construction
  /// @todo this should only be done if not flying! (currently done with has_entered)
  if (!taskpoint_start->has_entered()) {
    GeoPoint loc = taskpoint_start->get_location();
    update_stats_distances(loc, true);
    if (has_finish()) {
      /// @todo: call AbstractTask::update stats methods with fake state
      /// so stats are updated
    }
  }
}

// TIMES

fixed 
OrderedTask::scan_total_start_time(const AIRCRAFT_STATE &)
{
  if (taskpoint_start)
    return taskpoint_start->get_state_entered().Time;

  return fixed_zero;
}

fixed 
OrderedTask::scan_leg_start_time(const AIRCRAFT_STATE &)
{
  if (activeTaskPoint)
    return task_points[activeTaskPoint-1]->get_state_entered().Time;

  return -fixed_one;
}

// DISTANCES

fixed
OrderedTask::scan_distance_min(const GeoPoint &location, bool full)
{
  if (full) {
    SearchPoint ac(location, task_projection);
    dijkstra_min.distance_min(ac);
    m_location_min_last = location;
  }
  return taskpoint_start->scan_distance_min();
}

fixed
OrderedTask::scan_distance_max()
{
  if (task_points.empty()) // nothing to do!
    return fixed_zero;

  assert(activeTaskPoint < task_points.size());

  // for max calculations, since one can still travel further in the
  // sector, we pretend we are on the previous turnpoint so the
  // search samples will contain the full boundary
  const unsigned atp = activeTaskPoint;
  if (atp) {
    activeTaskPoint--;
    taskpoint_start->scan_active(task_points[activeTaskPoint]);
  }
  dijkstra_max.distance_max();

  if (atp) {
    activeTaskPoint = atp;
    taskpoint_start->scan_active(task_points[activeTaskPoint]);
  }
  return taskpoint_start->scan_distance_max();
}

void
OrderedTask::scan_distance_minmax(const GeoPoint &location, 
                                  bool force,
                                  fixed *dmin, fixed *dmax)
{
  if (!taskpoint_start)
    return;

  if (force)
    *dmax = scan_distance_max();

  bool force_min = force || distance_is_significant(location, m_location_min_last);
  *dmin = scan_distance_min(location, force_min);
}

fixed
OrderedTask::scan_distance_nominal()
{
  if (taskpoint_start)
    return taskpoint_start->scan_distance_nominal();

  return fixed_zero;
}

fixed
OrderedTask::scan_distance_scored(const GeoPoint &location)
{
  if (taskpoint_start)
    return taskpoint_start->scan_distance_scored(location);

  return fixed_zero;
}

fixed
OrderedTask::scan_distance_remaining(const GeoPoint &location)
{
  if (taskpoint_start)
    return taskpoint_start->scan_distance_remaining(location);

  return fixed_zero;
}

fixed
OrderedTask::scan_distance_travelled(const GeoPoint &location)
{
  if (taskpoint_start)
    return taskpoint_start->scan_distance_travelled(location);

  return fixed_zero;
}

fixed
OrderedTask::scan_distance_planned()
{
  if (taskpoint_start)
    return taskpoint_start->scan_distance_planned();

  return fixed_zero;
}

// TRANSITIONS

bool 
OrderedTask::check_transitions(const AIRCRAFT_STATE &state, 
                               const AIRCRAFT_STATE &state_last)
{
  if (!taskpoint_start)
    return false;

  taskpoint_start->scan_active(task_points[activeTaskPoint]);

  if (!state.Flying)
    return false;

  const int n_task = task_points.size();

  if (!n_task)
    return false;

  FlatBoundingBox bb_last(task_projection.project(state_last.Location),1);
  FlatBoundingBox bb_now(task_projection.project(state.Location),1);

  bool last_started = task_started();
  const bool last_finished = task_finished();

  const int t_min = max(0, (int)activeTaskPoint - 1);
  const int t_max = min(n_task - 1, (int)activeTaskPoint);
  bool full_update = false;

  for (int i = t_min; i <= t_max; i++) {

    bool transition_enter = false;
    bool transition_exit = false;

    if (i==0) {
      full_update |= check_transition_optional_start(state, state_last, bb_now, bb_last, 
                                                     transition_enter, transition_exit,
                                                     last_started);
    }

    full_update |= check_transition_point(*task_points[i], 
                                          state, state_last, bb_now, bb_last, 
                                          transition_enter, transition_exit,
                                          last_started, i==0);

    if (i == (int)activeTaskPoint) {
      const bool last_request_armed = task_advance.request_armed();

      if (task_advance.ready_to_advance(*task_points[i], state,
                                        transition_enter,
                                        transition_exit)) {
        task_advance.set_armed(false);
        
        if (i + 1 < n_task) {
          i++;
          setActiveTaskPoint(i);
          taskpoint_start->scan_active(task_points[activeTaskPoint]);
          
          task_events.active_advanced(*task_points[i], i);

          // on sector exit, must update samples since start sector
          // exit transition clears samples
          full_update = true;
        }
      } else if (!last_request_armed && task_advance.request_armed()) {
        task_events.request_arm(*task_points[i]);
      }
    }
  }

  taskpoint_start->scan_active(task_points[activeTaskPoint]);

  stats.task_finished = task_finished();
  stats.task_started = task_started();

  if (stats.task_started)
    taskpoint_finish->set_fai_finish_height(get_start_state().NavAltitude - fixed(1000));

  if (stats.task_started && !last_started)
    task_events.task_start();

  if (stats.task_finished && !last_finished)
    task_events.task_finish();

  return full_update;
}


bool
OrderedTask::check_transition_optional_start(const AIRCRAFT_STATE &state, 
                                             const AIRCRAFT_STATE &state_last,
                                             const FlatBoundingBox& bb_now,
                                             const FlatBoundingBox& bb_last,
                                             bool &transition_enter,
                                             bool &transition_exit,
                                             bool &last_started)
{
  bool full_update = false;
  for (unsigned j = 0; j < optional_start_points.size(); ++j) {
    full_update |= check_transition_point(*optional_start_points[j], 
                                          state, state_last, bb_now, bb_last, 
                                          transition_enter, transition_exit,
                                          last_started, true);

    if (transition_enter || transition_exit) {
      // we have entered or exited this optional start point, so select it.
      // user has no choice in this: rules for multiple start points are that
      // the last start OZ flown through is used for scoring
      
      select_optional_start(j);

      return full_update;
    }
  }
  return full_update;
}


bool
OrderedTask::check_transition_point(OrderedTaskPoint& point,
                                    const AIRCRAFT_STATE &state, 
                                    const AIRCRAFT_STATE &state_last,
                                    const FlatBoundingBox& bb_now,
                                    const FlatBoundingBox& bb_last,
                                    bool &transition_enter,
                                    bool &transition_exit,
                                    bool &last_started,
                                    const bool is_start)
{
  bool full_update = false;
  const bool nearby = point.boundingbox_overlaps(bb_now) || point.boundingbox_overlaps(bb_last);

  if (nearby && point.transition_enter(state, state_last)) {
    transition_enter = true;
    task_events.transition_enter(point);
  }
  
  if (nearby && point.transition_exit(state, state_last, task_projection)) {
    transition_exit = true;
    task_events.transition_exit(point);
    
    // detect restart
    if (is_start && last_started)
      last_started = false;
  }
  
  if (is_start) 
    update_start_transition(state, point);
  
  if (nearby) {
    if (point.update_sample_near(state, task_events, task_projection))
      full_update = true;
  } else {
    if (point.update_sample_far(state, task_events, task_projection))
      full_update = true;
  }
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

    calc_min_target(state, 
                    get_ordered_task_behaviour().aat_min_time + fixed(task_behaviour.optimise_targets_margin));

    if (task_behaviour.optimise_targets_bearing) {
      if (task_points[activeTaskPoint]->type == TaskPoint::AAT) {
        AATPoint *ap = (AATPoint *)task_points[activeTaskPoint];
        // very nasty hack
        TaskOptTarget tot(task_points, activeTaskPoint, state, glide_polar,
                          *ap, task_projection, taskpoint_start);
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

  if (!task_points[position])
    // nothing to do if this is deleted
    return;

  if (position >= task_points.size())
    // nothing to do
    return;

  if (position > 0)
    prev = task_points[position - 1];

  if (position + 1 < task_points.size())
    next = task_points[position + 1];

  task_points[position]->set_neighbours(prev, next);

  if (position==0) {
    for (unsigned i = 0; i < optional_start_points.size(); ++i) {
      optional_start_points[i]->set_neighbours(prev, next);
    }
  }
}

bool
OrderedTask::check_task() const
{
  return this->get_factory().validate();
}

OrderedTaskPoint*
OrderedTask::get_ordered_task_point(unsigned TPindex) const
{
 if (TPindex > task_points.size() - 1) {
   return NULL;
 }
 return task_points[TPindex];
}

AATPoint*
OrderedTask::get_AAT_task_point(unsigned TPindex) const
{
 if (TPindex > task_points.size() - 1) {
   return NULL;
 }
 if (task_points[TPindex]) {
    if (task_points[TPindex]->type == TaskPoint::AAT)
      return (AATPoint*) task_points[TPindex];
    else
      return (AATPoint*)NULL;
 }
 return NULL;
}

bool
OrderedTask::scan_start_finish()
{
  /// @todo also check there are not more than one start/finish point
  if (!task_points.size()) {
    taskpoint_start = NULL;
    taskpoint_finish = NULL;
    return false;
  }

  taskpoint_start = task_points[0]->type == TaskPoint::START
    ? (StartPoint *)task_points[0]
    : NULL;

  taskpoint_finish = task_points.size() > 1 && task_points[task_points.size() - 1]->type == TaskPoint::FINISH
    ? (FinishPoint *)task_points[task_points.size() - 1]
    : NULL;

  return has_start() && has_finish();
}

void
OrderedTask::erase(const unsigned index)
{
  delete task_points[index];
  task_points.erase(task_points.begin() + index);
}

void
OrderedTask::erase_optional_start(const unsigned index)
{
  delete optional_start_points[index];
  optional_start_points.erase(optional_start_points.begin() + index);
}

bool
OrderedTask::remove(const unsigned position)
{
  if (position >= task_points.size())
    return false;

  if (activeTaskPoint > position ||
      (activeTaskPoint > 0 && activeTaskPoint == task_points.size() - 1))
    activeTaskPoint--;

  erase(position);

  set_neighbours(position);
  if (position)
    set_neighbours(position - 1);

  update_geometry();
  return true;
}

bool
OrderedTask::remove_optional_start(const unsigned position)
{
  if (position >= optional_start_points.size())
    return false;

  erase_optional_start(position);

  if (task_points.size()>1)
    set_neighbours(0);

  update_geometry();
  return true;
}

bool 
OrderedTask::append(const OrderedTaskPoint &new_tp)
{
  if (/* is the new_tp allowed in this context? */
      (!task_points.empty() && !new_tp.predecessor_allowed()) ||
      /* can a tp be appended after the last one? */
      (task_points.size() >= 1 && !task_points[task_points.size() - 1]->successor_allowed()))
    return false;

  task_points.push_back(new_tp.clone(task_behaviour, m_ordered_behaviour));
  if (task_points.size() > 1)
    set_neighbours(task_points.size() - 2);
  else {
    // give it a value when we have one tp so it is not uninitialised
    m_location_min_last = new_tp.get_location();
  }

  set_neighbours(task_points.size() - 1);
  update_geometry();
  return true;
}

bool 
OrderedTask::append_optional_start(const OrderedTaskPoint &new_tp)
{
  optional_start_points.push_back(new_tp.clone(task_behaviour, m_ordered_behaviour));
  if (task_points.size() > 1)
    set_neighbours(0);
  update_geometry();
  return true;
}

bool 
OrderedTask::insert(const OrderedTaskPoint &new_tp,
                    const unsigned position)
{
  if (position >= task_points.size())
    return append(new_tp);

  if (/* is the new_tp allowed in this context? */
      (position > 0 && !new_tp.predecessor_allowed()) ||
      !new_tp.successor_allowed() ||
      /* can a tp be inserted at this position? */
      (position > 0 && !task_points[position - 1]->successor_allowed()) ||
      !task_points[position]->predecessor_allowed())
    return false;

  if (activeTaskPoint >= position)
    activeTaskPoint++;

  task_points.insert(task_points.begin() + position,
             new_tp.clone(task_behaviour, m_ordered_behaviour));

  if (position)
    set_neighbours(position - 1);

  set_neighbours(position);
  set_neighbours(position + 1);

  update_geometry();
  return true;
}

bool 
OrderedTask::replace(const OrderedTaskPoint &new_tp,
                     const unsigned position)
{
  if (position >= task_points.size())
    return false;

  if (task_points[position]->equals(&new_tp))
    // nothing to do
    return true;

  /* is the new_tp allowed in this context? */
  if ((position > 0 && !new_tp.predecessor_allowed()) ||
      (position + 1 < task_points.size() && !new_tp.successor_allowed()))
    return false;

  delete task_points[position];
  task_points[position] = new_tp.clone(task_behaviour, m_ordered_behaviour);

  if (position)
    set_neighbours(position - 1);

  set_neighbours(position);
  if (position + 1 < task_points.size())
    set_neighbours(position + 1);

  update_geometry();
  return true;
}


bool 
OrderedTask::replace_optional_start(const OrderedTaskPoint &new_tp,
                                    const unsigned position)
{
  if (position >= optional_start_points.size())
    return false;

  if (optional_start_points[position]->equals(&new_tp))
    // nothing to do
    return true;

  delete optional_start_points[position];
  optional_start_points[position] = new_tp.clone(task_behaviour, m_ordered_behaviour);

  set_neighbours(0);
  update_geometry();
  return true;
}


void 
OrderedTask::setActiveTaskPoint(unsigned index)
{
  if (index < task_points.size()) {
    if (activeTaskPoint != index)
      task_advance.set_armed(false);

    activeTaskPoint = index;
  } else if (task_points.empty()) {
    activeTaskPoint = 0;
  }
}

TaskWaypoint*
OrderedTask::getActiveTaskPoint() const
{
  if (activeTaskPoint < task_points.size())
    return task_points[activeTaskPoint];

  return NULL;
}

bool 
OrderedTask::validTaskPoint(const int index_offset) const
{
  unsigned index = activeTaskPoint + index_offset;
  return (index < task_points.size());
}


void
OrderedTask::glide_solution_remaining(const AIRCRAFT_STATE &aircraft,
                                      const GlidePolar &polar,
                                      GlideResult &total,
                                      GlideResult &leg)
{
  TaskMacCreadyRemaining tm(task_points, activeTaskPoint, polar);
  total = tm.glide_solution(aircraft);
  leg = tm.get_active_solution(aircraft);
  if (activeTaskPoint == 0)
    leg.Vector = GeoVector(aircraft.Location, taskpoint_start->get_location_remaining());
}

void
OrderedTask::glide_solution_travelled(const AIRCRAFT_STATE &aircraft, 
                                      GlideResult &total,
                                      GlideResult &leg)
{
  TaskMacCreadyTravelled tm(task_points, activeTaskPoint, glide_polar);
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
  TaskMacCreadyTotal tm(task_points, activeTaskPoint, glide_polar);
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
  TaskGlideRequired bgr(task_points, activeTaskPoint, aircraft, glide_polar);
  return bgr.search(fixed_zero);
}

bool
OrderedTask::calc_mc_best(const AIRCRAFT_STATE &aircraft, fixed& best) const
{
  // note setting of lower limit on mc
  TaskBestMc bmc(task_points,activeTaskPoint, aircraft, glide_polar);
  return bmc.search(glide_polar.get_mc(), best);
}


bool
OrderedTask::allow_incremental_boundary_stats(const AIRCRAFT_STATE &aircraft) const
{
  if (!activeTaskPoint)
    return false;
  assert(task_points[activeTaskPoint]);
  bool in_sector = task_points[activeTaskPoint]->isInSector(aircraft);
  if (activeTaskPoint>0) {
    in_sector |= task_points[activeTaskPoint-1]->isInSector(aircraft);
  }
  return (task_points[activeTaskPoint]->is_boundary_scored() || !in_sector);
}

bool
OrderedTask::calc_cruise_efficiency(const AIRCRAFT_STATE &aircraft, fixed& val) const
{
  if (allow_incremental_boundary_stats(aircraft)) {
    TaskCruiseEfficiency bce(task_points, activeTaskPoint, aircraft, glide_polar);
    val = bce.search(fixed_one);
    return true;
  } else {
    val = fixed_one;
    return false;
  }
}

bool 
OrderedTask::calc_effective_mc(const AIRCRAFT_STATE &aircraft, fixed& val) const
{
  if (allow_incremental_boundary_stats(aircraft)) {
    TaskEffectiveMacCready bce(task_points,activeTaskPoint, aircraft, glide_polar);
    val = bce.search(glide_polar.get_mc());
    return true;
  } else {
    val = glide_polar.get_mc();
    return false;
  }
}


fixed
OrderedTask::calc_min_target(const AIRCRAFT_STATE &aircraft, 
                             const fixed t_target) 
{
  if (stats.distance_max > stats.distance_min) {
    // only perform scan if modification is possible
    const fixed t_rem = max(fixed_zero, t_target - stats.total.TimeElapsed);

    TaskMinTarget bmt(task_points, activeTaskPoint, aircraft, glide_polar, t_rem, taskpoint_start);
    fixed p = bmt.search(fixed_zero);
    return p;
  }

  return fixed_zero;
}


fixed 
OrderedTask::calc_gradient(const AIRCRAFT_STATE &state) const
{
  if (task_points.size() < 1)
    return fixed_zero;

  // Iterate through remaining turnpoints
  fixed distance = fixed_zero;
  for (unsigned i = activeTaskPoint; i < task_points.size(); i++)
    // Sum up the leg distances
    distance += task_points[i]->get_vector_remaining(state).Distance;

  if (!distance)
    return fixed_zero;

  // Calculate gradient to the last turnpoint of the remaining task
  return (state.NavAltitude - task_points[task_points.size() - 1]->get_elevation()) / distance;
}

// Constructors/destructors

OrderedTask::~OrderedTask()
{
  for (OrderedTaskPointVector::iterator v = task_points.begin(); v != task_points.end();) {
    delete *v;
    task_points.erase(v);
  }
  for (OrderedTaskPointVector::iterator v = optional_start_points.begin(); v != optional_start_points.end();) {
    delete *v;
    optional_start_points.erase(v);
  }
  delete active_factory;
}

OrderedTask::OrderedTask(TaskEvents &te, 
                         const TaskBehaviour &tb,
                         const GlidePolar &gp,
                         const bool do_reserve):
  AbstractTask(ORDERED, te, tb, gp),
  taskpoint_start(NULL),
  taskpoint_finish(NULL),
  factory_mode(TaskBehaviour::FACTORY_RT),
  active_factory(NULL),
  m_ordered_behaviour(tb.ordered_defaults),
  task_advance(m_ordered_behaviour),
  dijkstra_min(*this, do_reserve),
  dijkstra_max(*this, do_reserve)
{
  active_factory = new RTTaskFactory(*this, task_behaviour);
  active_factory->update_ordered_task_behaviour(m_ordered_behaviour);
}

static void
Visit(const OrderedTask::OrderedTaskPointVector &points,
      TaskPointConstVisitor &visitor)
{
    const OrderedTask::OrderedTaskPointVector::const_iterator end = points.end();
    for (OrderedTask::OrderedTaskPointVector::const_iterator it = points.begin();
         it != end; ++it)
      visitor.Visit(**it);
}

static void
VisitReverse(const OrderedTask::OrderedTaskPointVector &points,
             TaskPointConstVisitor &visitor)
{
    const OrderedTask::OrderedTaskPointVector::const_reverse_iterator end = points.rend();
    for (OrderedTask::OrderedTaskPointVector::const_reverse_iterator it = points.rbegin();
         it != end; ++it)
      visitor.Visit(**it);
}

void 
OrderedTask::tp_CAccept(TaskPointConstVisitor& visitor, const bool reverse) const
{
  if (!reverse) {
    Visit(task_points, visitor);
  } else {
    VisitReverse(task_points, visitor);
  }
}

void 
OrderedTask::sp_CAccept(TaskPointConstVisitor& visitor, const bool reverse) const
{
  if (!reverse) {
    Visit(optional_start_points, visitor);
  } else {
    VisitReverse(optional_start_points, visitor);
  }
}

static void
Reset(OrderedTask::OrderedTaskPointVector &points)
{
    const OrderedTask::OrderedTaskPointVector::iterator end = points.end();
    for (OrderedTask::OrderedTaskPointVector::iterator it = points.begin();
         it != end; ++it)
      (*it)->reset();
}

void
OrderedTask::reset()
{  
  /// @todo also reset data in this class e.g. stats?
  Reset(task_points);
  Reset(optional_start_points);

  AbstractTask::reset();
  stats.task_finished = false;
  stats.task_started = false;
  task_advance.reset();
  setActiveTaskPoint(0);
}

const OrderedTaskPoint* 
OrderedTask::getTaskPoint(const unsigned index) const
{
  if (index >= task_points.size())
    return NULL;

  return task_points[index];
}

bool 
OrderedTask::task_finished() const
{
  if (taskpoint_finish)
    return (taskpoint_finish->has_entered());

  return false;
}

bool 
OrderedTask::task_started(bool soft) const
{
  if (taskpoint_start) {
    // have we really started?
    if (taskpoint_start->has_exited()) 
      return true;

    // if soft starts allowed, consider started if we progressed to next tp
    if (soft && (activeTaskPoint>0))
      return true;
  }

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
  return task_points[tp]->get_search_points();
}

void 
OrderedTask::set_tp_search_min(unsigned tp, const SearchPoint &sol) 
{
  if (!tp && !task_points[0]->has_exited())
    return;

  task_points[tp]->set_search_min(sol);
}

void 
OrderedTask::set_tp_search_achieved(unsigned tp, const SearchPoint &sol) 
{
  if (task_points[tp]->has_sampled())
    set_tp_search_min(tp, sol);
}

void 
OrderedTask::set_tp_search_max(unsigned tp, const SearchPoint &sol) 
{
  task_points[tp]->set_search_max(sol);
}

unsigned 
OrderedTask::task_size() const 
{
  return task_points.size();
}

unsigned 
OrderedTask::getActiveIndex() const 
{
  return activeTaskPoint;
}

void
OrderedTask::update_start_transition(const AIRCRAFT_STATE &state, OrderedTaskPoint& start)
{
  if (activeTaskPoint == 0) {
    // find boundary point that produces shortest
    // distance from state to that point to next tp point
    taskpoint_start->find_best_start(state, *task_points[1], task_projection);
  } else if (!start.has_exited() && !start.isInSector(state)) {
    start.reset();
    // reset on invalid transition to outside
    // point to nominal start point
  }
  // @todo: modify this for optional start?
}

AIRCRAFT_STATE 
OrderedTask::get_start_state() const
{
  if (has_start() && task_started()) 
    return taskpoint_start->get_state_entered();

  // @todo: modify this for optional start?

  AIRCRAFT_STATE null_state;
  return null_state;
}

AIRCRAFT_STATE 
OrderedTask::get_finish_state() const
{
  if (has_finish() && task_finished()) 
    return taskpoint_finish->get_state_entered();

  AIRCRAFT_STATE null_state;
  return null_state;
}

bool
OrderedTask::has_targets() const
{
  for (unsigned i = 0; i < task_points.size(); ++i)
    if (task_points[i]->has_target())
      return true;

  return false;
}

fixed
OrderedTask::get_finish_height() const
{
  if (taskpoint_finish)
    return taskpoint_finish->get_elevation();

  return fixed_zero;
}

GeoPoint 
OrderedTask::get_task_center(const GeoPoint& fallback_location) const
{
  if (!has_start() || !task_points[0])
    return fallback_location;

  return task_projection.get_center();
}

fixed 
OrderedTask::get_task_radius(const GeoPoint& fallback_location) const
{ 
  if (!has_start() || !task_points[0])
    return fixed_zero;

  return task_projection.ApproxRadius();
}

OrderedTask* 
OrderedTask::clone(TaskEvents &te, 
                   const TaskBehaviour &tb,
                   const GlidePolar &gp) const
{
  OrderedTask* new_task = new OrderedTask(te, tb, gp);

  new_task->m_ordered_behaviour = m_ordered_behaviour;

  new_task->set_factory(factory_mode);
  for (unsigned i = 0; i < task_points.size(); ++i) {
    new_task->append(*task_points[i]);
  }
  for (unsigned i = 0; i < optional_start_points.size(); ++i) {
    new_task->append_optional_start(*optional_start_points[i]);
  }
  new_task->activeTaskPoint = activeTaskPoint;
  new_task->update_geometry();
  return new_task;
}

void
OrderedTask::check_duplicate_waypoints(Waypoints& waypoints,
                                       OrderedTaskPointVector& points,
                                       const bool is_task)
{
  for (unsigned i = 0; i < points.size(); ++i) {
    const Waypoint &wp =
      waypoints.check_exists_or_append(points[i]->get_waypoint());

    const OrderedTaskPoint *new_tp = points[i]->clone(task_behaviour,
                                                      m_ordered_behaviour,
                                                      &wp);
    if (is_task)
      replace(*new_tp, i);
    else
      replace_optional_start(*new_tp, i);
    delete new_tp;
  }
}

void
OrderedTask::check_duplicate_waypoints(Waypoints& waypoints)
{
  check_duplicate_waypoints(waypoints, task_points, true);
  check_duplicate_waypoints(waypoints, optional_start_points, false);
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

  // ensure each task point made identical
  for (unsigned i = 0; i < that.task_size(); ++i) {
    if (i >= task_size()) {
      // that task is larger than this
      append(*that.task_points[i]);
      modified = true;
    } else if (!task_points[i]->equals(that.task_points[i])) {
      // that task point is changed
      replace(*that.task_points[i], i);
      modified = true;
    }
  }

  // remove if that optional start list is smaller than this one
  while (optional_start_points.size() > that.optional_start_points.size()) {
    remove_optional_start(optional_start_points.size() - 1);
    modified = true;
  }

  // ensure each task point made identical
  for (unsigned i = 0; i < that.optional_start_points.size(); ++i) {
    if (i >= optional_start_points.size()) {
      // that task is larger than this
      append_optional_start(*that.optional_start_points[i]);
      modified = true;
    } else if (!optional_start_points[i]->equals(that.optional_start_points[i])) {
      // that task point is changed
      replace_optional_start(*that.optional_start_points[i], i);
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
OrderedTask::relocate_optional_start(const unsigned position, const Waypoint& waypoint)
{
  if (position >= optional_start_points.size())
    return false;

  OrderedTaskPoint *new_tp = optional_start_points[position]->clone(task_behaviour,
                                                  m_ordered_behaviour,
                                                  &waypoint);
  delete optional_start_points[position];
  optional_start_points[position]= new_tp;
  return true;
}

bool
OrderedTask::relocate(const unsigned position, const Waypoint& waypoint) 
{
  if (position >= task_size())
    return false;

  OrderedTaskPoint *new_tp = task_points[position]->clone(task_behaviour,
                                                  m_ordered_behaviour,
                                                  &waypoint);
  bool success = replace(*new_tp, position);
  delete new_tp;
  return success;
}

TaskBehaviour::Factory_t
OrderedTask::set_factory(const TaskBehaviour::Factory_t the_factory)
{
  // detect no change
  if (factory_mode == the_factory)
    return factory_mode;

  if (the_factory != TaskBehaviour::FACTORY_MIXED) {
    // can switch from anything to mixed, otherwise need reset
    reset();

    /// @todo call into task_events to ask if reset is desired on
    /// factory change
  }
  factory_mode = the_factory;

  delete active_factory;

  switch (factory_mode) {
  case TaskBehaviour::FACTORY_RT:
    active_factory = new RTTaskFactory(*this, task_behaviour);
    break;
  case TaskBehaviour::FACTORY_FAI_GENERAL:
    active_factory = new FAITaskFactory(*this, task_behaviour);
    break;
  case TaskBehaviour::FACTORY_FAI_TRIANGLE:
    active_factory = new FAITriangleTaskFactory(*this, task_behaviour);
    break;
  case TaskBehaviour::FACTORY_FAI_OR:
    active_factory = new FAIORTaskFactory(*this, task_behaviour);
    break;
  case TaskBehaviour::FACTORY_FAI_GOAL:
    active_factory = new FAIGoalTaskFactory(*this, task_behaviour);
    break;
  case TaskBehaviour::FACTORY_AAT:
    active_factory = new AATTaskFactory(*this, task_behaviour);
    break;
  case TaskBehaviour::FACTORY_MIXED:
    active_factory = new MixedTaskFactory(*this, task_behaviour);
    break;
  case TaskBehaviour::FACTORY_TOURING:
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

std::vector<TaskBehaviour::Factory_t>
OrderedTask::get_factory_types(bool all) const
{
  /// @todo: check transform types if all=false
  std::vector<TaskBehaviour::Factory_t> f_list;
  f_list.push_back(TaskBehaviour::FACTORY_RT);
  f_list.push_back(TaskBehaviour::FACTORY_AAT);
  f_list.push_back(TaskBehaviour::FACTORY_FAI_GENERAL);
  return f_list;
}

void 
OrderedTask::clear()
{
  while (task_points.size())
    erase(0);

  while (optional_start_points.size())
    erase_optional_start(0);

  reset();
  m_ordered_behaviour = task_behaviour.ordered_defaults;
}

OrderedTaskPoint* 
OrderedTask::get_tp(const unsigned position)
{
  if (position >= task_size())
    return NULL;

  return task_points[position];
}

const OrderedTaskPoint* 
OrderedTask::get_tp(const unsigned position) const
{
  if (position >= task_size())
    return NULL;

  return task_points[position];
}

FlatBoundingBox 
OrderedTask::get_bounding_box(const GeoBounds& bounds) const
{
  if (!task_size()) {
    // undefined!
    return FlatBoundingBox(FlatGeoPoint(0,0),FlatGeoPoint(0,0));
  }
  FlatGeoPoint ll = task_projection.project(GeoPoint(bounds.west, bounds.south));
  FlatGeoPoint lr = task_projection.project(GeoPoint(bounds.east, bounds.south));
  FlatGeoPoint ul = task_projection.project(GeoPoint(bounds.west, bounds.north));
  FlatGeoPoint ur = task_projection.project(GeoPoint(bounds.east, bounds.north));
  FlatGeoPoint fmin(min(ll.Longitude,ul.Longitude), min(ll.Latitude,lr.Latitude));
  FlatGeoPoint fmax(max(lr.Longitude,ur.Longitude), max(ul.Latitude,ur.Latitude));
  // note +/- 1 to ensure rounding keeps bb valid 
  fmin.Longitude-= 1; fmin.Latitude-= 1;
  fmax.Longitude+= 1; fmax.Latitude+= 1;
  return FlatBoundingBox (fmin, fmax);
}

FlatBoundingBox 
OrderedTask::get_bounding_box(const GeoPoint& point) const
{
  if (!task_size()) {
    // undefined!
    return FlatBoundingBox(FlatGeoPoint(0,0),FlatGeoPoint(0,0));
  }
  return FlatBoundingBox (task_projection.project(point), 1);
}

void 
OrderedTask::rotateOptionalStarts()
{
  if (!task_size())
    return;
  if (!optional_start_points.size()) 
    return;

  select_optional_start(0);
}

void
OrderedTask::select_optional_start(unsigned pos) 
{
  assert(pos< optional_start_points.size());

  // put task start onto end
  optional_start_points.push_back(task_points[0]);
  // set task start from top optional item
  task_points[0] = optional_start_points[pos];
  // remove top optional item from list
  optional_start_points.erase(optional_start_points.begin()+pos);

  // update neighbour links
  set_neighbours(0);
  if (task_points.size()>1)
    set_neighbours(1);

  // we've changed the task, so update geometry
  update_geometry();
}

const OrderedTaskPoint *
OrderedTask::get_optional_start(unsigned pos) const
{
  if (pos >= optional_start_points.size())
    return NULL;

  return optional_start_points[pos];
}

void
OrderedTask::update_summary(TaskSummary& ordered_summary) const
{
  ordered_summary.clear();

  ordered_summary.active = activeTaskPoint;
  for (unsigned i = 0; i < task_points.size(); ++i) {    
    TaskSummaryPoint tsp;
    tsp.d_planned = task_points[i]->get_vector_planned().Distance;
    if (i==0) {
      tsp.achieved = task_points[i]->has_exited();
    } else {
      tsp.achieved = task_points[i]->has_sampled();
    }
    ordered_summary.append(tsp);
  }
  ordered_summary.update(stats.total.remaining.get_distance(), 
                         stats.total.planned.get_distance());
}

unsigned 
OrderedTask::optional_starts_size() const {
  return optional_start_points.size();
}
