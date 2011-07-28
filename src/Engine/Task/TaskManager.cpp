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
#include "TaskManager.hpp"
#include "Visitors/TaskPointVisitor.hpp"
#include "Sizes.h"
#include "Tasks/TaskSolvers/TaskSolution.hpp"
#include "Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Util/StringUtil.hpp"

// uses delegate pattern


TaskManager::TaskManager(TaskEvents &te,
                         const Waypoints &wps): 
  m_glide_polar(fixed_zero),
  trace_full(60),
  trace_sprint(0, 9000, 300),
  task_ordered(te, task_behaviour, m_glide_polar, true),
  task_goto(te, task_behaviour, m_glide_polar, wps),
  task_abort(te, task_behaviour, m_glide_polar, wps),
  contest_manager(task_behaviour.contest, 
                  task_behaviour.contest_handicap,
                  trace_full, trace_sprint),
  mode(MODE_NULL),
  active_task(NULL) {
  null_stats.reset();
}

void
TaskManager::set_task_behaviour(const TaskBehaviour& behaviour)
{
  task_behaviour = behaviour;

  task_ordered.SetTaskBehaviour(behaviour);
  task_goto.SetTaskBehaviour(behaviour);
  task_abort.SetTaskBehaviour(behaviour);
}

TaskManager::TaskMode_t 
TaskManager::set_mode(const TaskMode_t the_mode)
{
  switch(the_mode) {
  case (MODE_ABORT):
    active_task = &task_abort;
    mode = MODE_ABORT;
    break;

  case (MODE_ORDERED):
    if (task_ordered.task_size()) {
      active_task = &task_ordered;
      mode = MODE_ORDERED;
      break;
    }

  case (MODE_GOTO):
    if (task_goto.getActiveTaskPoint()) {
      active_task = &task_goto;
      mode = MODE_GOTO;
      break;
    }

  case (MODE_NULL):
    active_task = NULL;
    mode = MODE_NULL;
    break;
  };
  return mode;
}

void 
TaskManager::setActiveTaskPoint(unsigned index)
{
  if (active_task)
    active_task->setActiveTaskPoint(index);
}

unsigned 
TaskManager::getActiveTaskPointIndex() const
{
  if (active_task)
    return active_task->getActiveTaskPointIndex();

  return 0;
}

void 
TaskManager::incrementActiveTaskPoint(int offset)
{
  if (active_task) {
    unsigned i = getActiveTaskPointIndex();
    if ((int)i+offset<0) { // prevent wrap-around
      if (mode == MODE_ORDERED)
        task_ordered.rotateOptionalStarts();
      else
        setActiveTaskPoint(0);
    } else {
      setActiveTaskPoint(i+offset);
    }
  }
}

TaskWaypoint*
TaskManager::getActiveTaskPoint() const
{
  if (active_task) 
    return active_task->getActiveTaskPoint();

  return NULL;
}

void
TaskManager::update_common_stats_times(const AIRCRAFT_STATE &state)
{
  if (task_ordered.task_size() > 1) {
    common_stats.task_started = task_ordered.get_stats().task_started;
    common_stats.task_finished = task_ordered.get_stats().task_finished;

    common_stats.ordered_has_targets = task_ordered.has_targets();

    common_stats.aat_time_remaining =
        max(fixed_zero, task_ordered.get_ordered_task_behaviour().aat_min_time -
                        task_ordered.get_stats().total.TimeElapsed);

    if (positive(common_stats.aat_time_remaining))
      common_stats.aat_speed_remaining =
          fixed(task_ordered.get_stats().total.remaining.get_distance()) /
          common_stats.aat_time_remaining;
    else
      common_stats.aat_speed_remaining = -fixed_one;

    fixed aat_min_time = task_ordered.get_ordered_task_behaviour().aat_min_time;

    if (positive(aat_min_time)) {
      common_stats.aat_speed_max =
          task_ordered.get_stats().distance_max / aat_min_time;
      common_stats.aat_speed_min =
          task_ordered.get_stats().distance_min / aat_min_time;
    } else {
      common_stats.aat_speed_max = -fixed_one;
      common_stats.aat_speed_min = -fixed_one;
    }

    common_stats.task_time_remaining =
        task_ordered.get_stats().total.TimeRemaining;
    common_stats.task_time_elapsed =
        task_ordered.get_stats().total.TimeElapsed;

    const fixed start_max_height =
        fixed(task_ordered.get_ordered_task_behaviour().start_max_height) +
        fixed(task_ordered.get_ordered_task_behaviour().start_max_height_ref ?
              0 : task_ordered.get_tp(0)->get_elevation());
    if (positive(start_max_height) && state.flying) {
      if (!positive(common_stats.TimeUnderStartMaxHeight) &&
          state.altitude < start_max_height) {
        common_stats.TimeUnderStartMaxHeight = state.Time;
      }
      if (state.altitude > start_max_height) {
          common_stats.TimeUnderStartMaxHeight = -fixed_one;
      }
    } else {
      common_stats.TimeUnderStartMaxHeight = -fixed_one;
    }

    task_ordered.update_summary(common_stats.ordered_summary);

  } else {
    common_stats.reset_task();
  }
}

void
TaskManager::update_common_stats_waypoints(const AIRCRAFT_STATE &state)
{
  common_stats.vector_home = task_abort.get_vector_home(state);

  common_stats.landable_reachable = task_abort.has_landable_reachable();

  common_stats.next_solution.Reset();
  if (active_task) {
    const TaskWaypoint* tp= active_task->getActiveTaskPoint();
    if (tp != NULL) {
      // must make an UnorderedTaskPoint here so we pick up arrival height requirements
      UnorderedTaskPoint fp(tp->get_waypoint(), task_behaviour);
      GlidePolar polar = m_glide_polar; 
      // @todo: consider change to task_abort.get_safety_polar(); 
      common_stats.next_solution = TaskSolution::glide_solution_remaining(fp, state, polar);
    }
  }
}

void
TaskManager::update_common_stats_task(const AIRCRAFT_STATE &state)
{
  common_stats.mode_abort = (mode == MODE_ABORT);
  common_stats.mode_goto = (mode == MODE_GOTO);
  common_stats.mode_ordered = (mode == MODE_ORDERED);

  common_stats.ordered_valid = task_ordered.check_task();

  if (common_stats.ordered_valid) {
    common_stats.ordered_has_optional_starts = task_ordered.optional_starts_size()>0;
  } else {
    common_stats.ordered_has_optional_starts = false;
  }

  if (active_task && active_task->get_stats().task_valid) {
    common_stats.active_has_next = active_task->validTaskPoint(1);
    common_stats.active_has_previous = active_task->validTaskPoint(-1);
    common_stats.next_is_last = active_task->validTaskPoint(1) &&
                                !active_task->validTaskPoint(2);
    common_stats.previous_is_first = active_task->validTaskPoint(-1) &&
                                     !active_task->validTaskPoint(-2);
    common_stats.active_taskpoint_index = this->active_task->getActiveTaskPointIndex();
  } else {
    common_stats.active_has_next = false;
    common_stats.active_has_previous = false;
    common_stats.next_is_last = false;
    common_stats.previous_is_first = false;
    common_stats.active_taskpoint_index = 0;
  }
}

void
TaskManager::update_common_stats_polar(const AIRCRAFT_STATE &state)
{
  common_stats.current_mc = m_glide_polar.GetMC();
  common_stats.current_bugs = m_glide_polar.GetBugs();
  common_stats.current_ballast = m_glide_polar.GetBallast();

  common_stats.current_risk_mc = 
    m_glide_polar.GetRiskMC(state.working_band_fraction, 
                          task_behaviour.risk_gamma);

  GlidePolar risk_polar = m_glide_polar;
  risk_polar.SetMC(common_stats.current_risk_mc);

  common_stats.V_block = 
    m_glide_polar.SpeedToFly(state,
                               get_stats().current_leg.solution_remaining,
                               true);

  // note right now we only use risk mc for dolphin speeds

  common_stats.V_dolphin = 
    risk_polar.SpeedToFly(state,
                            get_stats().current_leg.solution_remaining,
                            false);
}

void
TaskManager::update_common_stats(const AIRCRAFT_STATE &state)
{
  update_common_stats_times(state);
  update_common_stats_task(state);
  update_common_stats_waypoints(state);
  update_common_stats_polar(state);
}

bool 
TaskManager::update(const AIRCRAFT_STATE &state, 
                    const AIRCRAFT_STATE& state_last)
{
  // always update ordered task so even if we are temporarily
  // in a different mode, so the task stats are still updated.
  // Otherwise, the task stats would freeze and sampling etc would
  // not be performed.  In actual use, even if you are in Abort/Goto
  // you still may want to go back to the task and have it know where
  // you went with respect to your task turnpoints etc.

  bool retval = false;

  if (state_last.Time > state.Time)
    reset();

  if (state.flying) {
    // either olc or basic trace requires trace_full
    if (task_behaviour.enable_olc || task_behaviour.enable_trace)
      trace_full.append(state);
    // only olc requires trace_sprint
    if (task_behaviour.enable_olc)
      trace_sprint.append(state);
  }

  if (task_ordered.task_size() > 1)
    // always update ordered task
    retval |= task_ordered.update(state, state_last);

  // inform the abort task whether it is running as the task or not  
  task_abort.set_active(active_task == &task_abort);
  // and tell it where the task destination is (if any)
  if (task_behaviour.abort_task_mode == atmSimple)
    task_abort.set_task_destination(state, NULL);
  else if (task_behaviour.abort_task_mode == atmHome)
    task_abort.set_task_destination_home(state);
  else
    task_abort.set_task_destination(state, getActiveTaskPoint());

  retval |= task_abort.update(state, state_last);

  if (active_task 
      && (active_task != &task_ordered)
      && (active_task != &task_abort))
    // update mode task for any that have not yet run
    retval |= active_task->update(state, state_last);

  update_common_stats(state);

  return retval;
}

bool 
TaskManager::update_idle(const AIRCRAFT_STATE& state)
{
  bool retval = false;

  if (state.flying) {
    // Update the Traces even if contest
    // optimization is not _currently_ enabled
    if (task_behaviour.enable_olc || task_behaviour.enable_trace)
      retval |= trace_full.optimise_if_old();

    if (task_behaviour.enable_olc)
      retval |= trace_sprint.optimise_if_old();

    // If contest optimization is enabled
    // -> Optimize
    if (task_behaviour.enable_olc)
      retval |= contest_manager.update_idle();
  }

  if (active_task)
    retval |= active_task->update_idle(state);

  return retval;
}

bool
TaskManager::score_exhaustive()
{
  return task_behaviour.enable_olc &&
    contest_manager.solve_exhaustive();
}

const TaskStats& 
TaskManager::get_stats() const
{
  if (active_task)
    return active_task->get_stats();

  return null_stats;
}

bool
TaskManager::do_goto(const Waypoint & wp)
{
  if (task_goto.do_goto(wp)) {
    set_mode(MODE_GOTO);
    return true;
  }

  return false;
}

bool 
TaskManager::check_task() const
{
  if (active_task) 
    return active_task->check_task();

  return false;
}

void
TaskManager::reset()
{
  task_ordered.reset();
  task_goto.reset();
  task_abort.reset();
  contest_manager.reset();
  common_stats.reset();
  m_glide_polar.SetCruiseEfficiency(fixed_one);
  trace_full.clear();
  trace_sprint.clear();
}

unsigned 
TaskManager::task_size() const
{
  if (active_task)
    return active_task->task_size();

  return 0;
}

GeoPoint 
TaskManager::random_point_in_task(const unsigned index, const fixed mag) const
{
  if (active_task == &task_ordered && index < task_size())
    return task_ordered.getTaskPoint(index)->randomPointInSector(mag);

  if (index <= task_size())
    return active_task->getActiveTaskPoint()->get_location();

  GeoPoint null_location(Angle::zero(), Angle::zero());
  return null_location;
}

void 
TaskManager::set_glide_polar(const GlidePolar& glide_polar) 
{
  m_glide_polar = glide_polar;
}

fixed 
TaskManager::get_finish_height() const
{
  if (active_task)
    return active_task->get_finish_height();

  return fixed_zero;
}

bool 
TaskManager::update_auto_mc(const AIRCRAFT_STATE& state_now,
                            const fixed fallback_mc)
{
  if (active_task &&
      active_task->update_auto_mc(m_glide_polar, state_now, fallback_mc))
    return true;

  if (!task_behaviour.auto_mc) 
    return false;

  if (task_behaviour.auto_mc_mode == TaskBehaviour::AUTOMC_FINALGLIDE)
    return false;

  if (positive(fallback_mc)) {
    m_glide_polar.SetMC(fallback_mc);
    return true;
  }

  return false;
}

GeoPoint
TaskManager::get_task_center(const GeoPoint& fallback_location) const
{
  if (active_task)
    return active_task->get_task_center(fallback_location);

  return fallback_location;
}

fixed
TaskManager::get_task_radius(const GeoPoint& fallback_location) const
{
  if (active_task)
    return active_task->get_task_radius(fallback_location);

  return fixed_zero;
}

const TCHAR*
TaskManager::get_ordered_taskpoint_name(unsigned TPindex) const
{
 static TCHAR buff[NAME_SIZE+1];
 buff[0] = '\0';

 if (!check_ordered_task())
   return buff;

 if (active_task == &task_ordered && TPindex < task_size())
   CopyString(buff, task_ordered.getTaskPoint(TPindex)->get_waypoint().Name.c_str(),
              NAME_SIZE + 1);

 return buff;
}

bool
TaskManager::isInSector (const unsigned TPindex, const AIRCRAFT_STATE &ref,
                          const bool AATOnly) const
{
  if (!check_ordered_task())
    return false;

  if (AATOnly) {
    const AATPoint *ap = task_ordered.get_AAT_task_point(TPindex);
    if (ap)
      return ap->isInSector(ref);
  }
  else {
    const OrderedTaskPoint *p = task_ordered.getTaskPoint(TPindex);
    if (p)
      return p->isInSector(ref);
  }

  return false;
}

const GeoPoint&
TaskManager::get_location_target(const unsigned TPindex, const GeoPoint& fallback_location) const
{
  if (!check_ordered_task())
    return fallback_location;

  const AATPoint *ap = task_ordered.get_AAT_task_point(TPindex);
  if (ap)
    return ap->get_location_target();

 return fallback_location;
}
bool
TaskManager::target_is_locked(const unsigned TPindex) const
{
  if (!check_ordered_task())
    return false;

  const AATPoint *ap = task_ordered.get_AAT_task_point(TPindex);
  if (ap)
    return ap->target_is_locked();

 return false;
}

bool
TaskManager::has_target(const unsigned TPindex) const
{
  if (!check_ordered_task())
    return false;

  const AATPoint *ap = task_ordered.get_AAT_task_point(TPindex);
  if (ap)
    return ap->has_target();

 return false;
}

bool
TaskManager::set_target(const unsigned TPindex, const GeoPoint &loc,
   const bool override_lock)
{
  if (!check_ordered_task())
    return false;

  AATPoint *ap = task_ordered.get_AAT_task_point(TPindex);
  if (ap)
    ap->set_target(loc, override_lock);

  return true;
}

bool
TaskManager::set_target(const unsigned TPindex, const fixed range,
   const fixed radial)
{
  if (!check_ordered_task())
    return false;

  AATPoint *ap = task_ordered.get_AAT_task_point(TPindex);
  if (ap)
    ap->set_target(range, radial, task_ordered.get_task_projection());

  return true;
}

bool
TaskManager::get_target_range_radial(const unsigned TPindex, fixed &range,
   fixed &radial) const
{
  if (!check_ordered_task())
    return false;

  const AATPoint *ap = task_ordered.get_AAT_task_point(TPindex);
  if (ap)
    ap->get_target_range_radial(range, radial);

  return true;
}

bool
TaskManager::target_lock(const unsigned TPindex, bool do_lock)
{
  if (!check_ordered_task())
    return false;

  AATPoint *ap = task_ordered.get_AAT_task_point(TPindex);
  if (ap)
    ap->target_lock(do_lock);

  return true;
}

const GeoPoint&
TaskManager::get_ordered_taskpoint_location(const unsigned TPindex,
   const GeoPoint& fallback_location) const
{
  if (!check_ordered_task())
    return fallback_location;

  const TaskWaypoint *tp = task_ordered.getTaskPoint(TPindex);
  if (tp)
    return tp->get_location();

  return fallback_location;
}

fixed
TaskManager::get_ordered_taskpoint_radius(const unsigned TPindex) const
{
  if (!check_ordered_task())
    return fixed(5);

  const OrderedTaskPoint *otp = task_ordered.getTaskPoint(TPindex);
  if (otp) {
    ObservationZonePoint *ozp = otp->get_oz();

    switch (ozp->shape) {
    case ObservationZonePoint::LINE:
    case ObservationZonePoint::CYLINDER:
    case ObservationZonePoint::SECTOR:
    case ObservationZonePoint::FAI_SECTOR:
    case ObservationZonePoint::KEYHOLE:
    case ObservationZonePoint::BGAFIXEDCOURSE:
    case ObservationZonePoint::BGAENHANCEDOPTION:
    case ObservationZonePoint::BGA_START:
    case ObservationZonePoint::ANNULAR_SECTOR:
      CylinderZone *cz = (CylinderZone *) ozp;
      if (cz)
        return cz->getRadius();
      break;
    }
  }
  return fixed(5);
}

OrderedTask* 
TaskManager::clone(TaskEvents &te, const TaskBehaviour &tb,
                   const GlidePolar &gp) const
{
  return task_ordered.clone(te, tb, gp);
}

bool 
TaskManager::commit(const OrderedTask& other)
{
  bool retval = task_ordered.commit(other);

  if (other.task_size()) {
    // if valid, resume the task
    switch (mode) {
    case MODE_NULL:
      set_mode(MODE_ORDERED); // set mode first
      setActiveTaskPoint(0); // then set tp
      break;
    case MODE_GOTO:
    case MODE_ABORT:
      // resume on load
      set_mode(MODE_ORDERED);
      break;
    case MODE_ORDERED:
      incrementActiveTaskPoint(0); // ensure tp is within size
      break;
    };
  } else if (mode == MODE_ORDERED) {
    setActiveTaskPoint(0); // reset tp of ordered task so will be zero
                           // on next load if valid
    set_mode(MODE_NULL);
  }

  return retval;
}

void
TaskManager::takeoff_autotask(const GeoPoint& loc, const fixed terrain_alt)
{
  // create a goto task on takeoff
  if (!active_task && task_goto.takeoff_autotask(loc, terrain_alt))
    set_mode(MODE_GOTO);
}
