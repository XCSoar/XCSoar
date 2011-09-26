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
  glide_polar(fixed_zero),
  task_ordered(te, task_behaviour, glide_polar, true),
  task_goto(te, task_behaviour, glide_polar, wps),
  task_abort(te, task_behaviour, glide_polar, wps),
  mode(MODE_NULL),
  active_task(NULL) {
  null_stats.reset();
}

void
TaskManager::SetTaskBehaviour(const TaskBehaviour& behaviour)
{
  task_behaviour = behaviour;

  task_ordered.SetTaskBehaviour(behaviour);
  task_goto.SetTaskBehaviour(behaviour);
  task_abort.SetTaskBehaviour(behaviour);
}

TaskManager::TaskMode 
TaskManager::SetMode(const TaskMode _mode)
{
  switch(_mode) {
  case (MODE_ABORT):
    active_task = &task_abort;
    mode = MODE_ABORT;
    break;

  case (MODE_ORDERED):
    if (task_ordered.TaskSize()) {
      active_task = &task_ordered;
      mode = MODE_ORDERED;
      break;
    }

  case (MODE_GOTO):
    if (task_goto.GetActiveTaskPoint()) {
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
TaskManager::SetActiveTaskPoint(unsigned index)
{
  if (active_task)
    active_task->SetActiveTaskPoint(index);
}

unsigned 
TaskManager::GetActiveTaskPointIndex() const
{
  if (active_task)
    return active_task->GetActiveTaskPointIndex();

  return 0;
}

void 
TaskManager::IncrementActiveTaskPoint(int offset)
{
  if (active_task) {
    unsigned i = GetActiveTaskPointIndex();
    if ((int)i+offset<0) { // prevent wrap-around
      if (mode == MODE_ORDERED)
        task_ordered.RotateOptionalStarts();
      else
        SetActiveTaskPoint(0);
    } else {
      SetActiveTaskPoint(i+offset);
    }
  }
}

TaskWaypoint*
TaskManager::GetActiveTaskPoint() const
{
  if (active_task) 
    return active_task->GetActiveTaskPoint();

  return NULL;
}

void
TaskManager::UpdateCommonStatsTimes(const AircraftState &state)
{
  if (task_ordered.TaskSize() > 1) {
    common_stats.task_started = task_ordered.GetStats().task_started;
    common_stats.task_finished = task_ordered.GetStats().task_finished;

    common_stats.ordered_has_targets = task_ordered.HasTargets();

    common_stats.aat_time_remaining =
        max(fixed_zero, task_ordered.get_ordered_task_behaviour().aat_min_time -
                        task_ordered.GetStats().total.time_elapsed);

    if (positive(common_stats.aat_time_remaining))
      common_stats.aat_speed_remaining =
          fixed(task_ordered.GetStats().total.remaining.get_distance()) /
          common_stats.aat_time_remaining;
    else
      common_stats.aat_speed_remaining = -fixed_one;

    fixed aat_min_time = task_ordered.get_ordered_task_behaviour().aat_min_time;

    if (positive(aat_min_time)) {
      common_stats.aat_speed_max =
          task_ordered.GetStats().distance_max / aat_min_time;
      common_stats.aat_speed_min =
          task_ordered.GetStats().distance_min / aat_min_time;
    } else {
      common_stats.aat_speed_max = -fixed_one;
      common_stats.aat_speed_min = -fixed_one;
    }

    common_stats.task_time_remaining =
        task_ordered.GetStats().total.time_remaining;
    common_stats.task_time_elapsed =
        task_ordered.GetStats().total.time_elapsed;

    const fixed start_max_height =
        fixed(task_ordered.get_ordered_task_behaviour().start_max_height) +
        fixed(task_ordered.get_ordered_task_behaviour().start_max_height_ref
              == hrMSL ? fixed_zero : task_ordered.get_tp(0)->GetElevation());
    if (positive(start_max_height) && state.flying) {
      if (!positive(common_stats.TimeUnderStartMaxHeight) &&
          state.altitude < start_max_height) {
        common_stats.TimeUnderStartMaxHeight = state.time;
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
TaskManager::UpdateCommonStatsWaypoints(const AircraftState &state)
{
  common_stats.vector_home = task_abort.get_vector_home(state);

  common_stats.landable_reachable = task_abort.has_landable_reachable();

  common_stats.next_solution.Reset();
  if (active_task) {
    const TaskWaypoint* tp= active_task->GetActiveTaskPoint();
    if (tp != NULL) {
      // must make an UnorderedTaskPoint here so we pick up arrival height requirements
      UnorderedTaskPoint fp(tp->GetWaypoint(), task_behaviour);
      GlidePolar polar = glide_polar; 
      // @todo: consider change to task_abort.get_safety_polar(); 
      common_stats.next_solution = TaskSolution::glide_solution_remaining(fp, state, polar);
    }
  }
}

void
TaskManager::UpdateCommonStatsTask(const AircraftState &state)
{
  common_stats.mode_abort = (mode == MODE_ABORT);
  common_stats.mode_goto = (mode == MODE_GOTO);
  common_stats.mode_ordered = (mode == MODE_ORDERED);

  common_stats.ordered_valid = task_ordered.CheckTask();

  if (common_stats.ordered_valid) {
    common_stats.ordered_has_optional_starts =
        task_ordered.OptionalStartsSize() > 0;
  } else {
    common_stats.ordered_has_optional_starts = false;
  }

  if (active_task && active_task->GetStats().task_valid) {
    common_stats.active_has_next = active_task->IsValidTaskPoint(1);
    common_stats.active_has_previous = active_task->IsValidTaskPoint(-1);
    common_stats.next_is_last = active_task->IsValidTaskPoint(1) &&
                                !active_task->IsValidTaskPoint(2);
    common_stats.previous_is_first = active_task->IsValidTaskPoint(-1) &&
                                     !active_task->IsValidTaskPoint(-2);
    common_stats.active_taskpoint_index = this->active_task->GetActiveTaskPointIndex();
  } else {
    common_stats.active_has_next = false;
    common_stats.active_has_previous = false;
    common_stats.next_is_last = false;
    common_stats.previous_is_first = false;
    common_stats.active_taskpoint_index = 0;
  }
}

void
TaskManager::UpdateCommonStatsPolar(const AircraftState &state)
{
  common_stats.current_mc = glide_polar.GetMC();
  common_stats.current_bugs = glide_polar.GetBugs();
  common_stats.current_ballast = glide_polar.GetBallast();

  common_stats.current_risk_mc = 
    glide_polar.GetRiskMC(state.working_band_fraction, 
                          task_behaviour.risk_gamma);

  GlidePolar risk_polar = glide_polar;
  risk_polar.SetMC(common_stats.current_risk_mc);

  common_stats.V_block = 
    glide_polar.SpeedToFly(state,
                               GetStats().current_leg.solution_remaining,
                               true);

  // note right now we only use risk mc for dolphin speeds

  common_stats.V_dolphin = 
    risk_polar.SpeedToFly(state,
                            GetStats().current_leg.solution_remaining,
                            false);
}

void
TaskManager::UpdateCommonStats(const AircraftState &state)
{
  UpdateCommonStatsTimes(state);
  UpdateCommonStatsTask(state);
  UpdateCommonStatsWaypoints(state);
  UpdateCommonStatsPolar(state);
}

bool 
TaskManager::Update(const AircraftState &state, 
                    const AircraftState &state_last)
{
  // always update ordered task so even if we are temporarily
  // in a different mode, so the task stats are still updated.
  // Otherwise, the task stats would freeze and sampling etc would
  // not be performed.  In actual use, even if you are in Abort/Goto
  // you still may want to go back to the task and have it know where
  // you went with respect to your task turnpoints etc.

  bool retval = false;

  if (state_last.time > state.time)
    Reset();

  if (task_ordered.TaskSize() > 1)
    // always update ordered task
    retval |= task_ordered.Update(state, state_last);

  // inform the abort task whether it is running as the task or not  
  task_abort.set_active(active_task == &task_abort);
  // and tell it where the task destination is (if any)
  if (task_behaviour.abort_task_mode == atmSimple)
    task_abort.set_task_destination(state, NULL);
  else if (task_behaviour.abort_task_mode == atmHome)
    task_abort.set_task_destination_home(state);
  else
    task_abort.set_task_destination(state, GetActiveTaskPoint());

  retval |= task_abort.Update(state, state_last);

  if (active_task 
      && (active_task != &task_ordered)
      && (active_task != &task_abort))
    // update mode task for any that have not yet run
    retval |= active_task->Update(state, state_last);

  UpdateCommonStats(state);

  return retval;
}

bool 
TaskManager::UpdateIdle(const AircraftState& state)
{
  bool retval = false;

  if (active_task)
    retval |= active_task->UpdateIdle(state);

  return retval;
}

const TaskStats& 
TaskManager::GetStats() const
{
  if (active_task)
    return active_task->GetStats();

  return null_stats;
}

bool
TaskManager::DoGoto(const Waypoint &wp)
{
  if (task_goto.DoGoto(wp)) {
    SetMode(MODE_GOTO);
    return true;
  }

  return false;
}

bool 
TaskManager::CheckTask() const
{
  if (active_task) 
    return active_task->CheckTask();

  return false;
}

void
TaskManager::Reset()
{
  task_ordered.Reset();
  task_goto.Reset();
  task_abort.Reset();
  common_stats.reset();
  glide_polar.SetCruiseEfficiency(fixed_one);
}

unsigned 
TaskManager::TaskSize() const
{
  if (active_task)
    return active_task->TaskSize();

  return 0;
}

GeoPoint 
TaskManager::RandomPointInTask(const unsigned index, const fixed mag) const
{
  if (active_task == &task_ordered && index < TaskSize())
    return task_ordered.GetTaskPoint(index)->randomPointInSector(mag);

  if (index <= TaskSize())
    return active_task->GetActiveTaskPoint()->GetLocation();

  GeoPoint null_location(Angle::Zero(), Angle::Zero());
  return null_location;
}

void 
TaskManager::SetGlidePolar(const GlidePolar &_glide_polar)
{
  glide_polar = _glide_polar;
}

fixed 
TaskManager::GetFinishHeight() const
{
  if (active_task)
    return active_task->GetFinishHeight();

  return fixed_zero;
}

bool 
TaskManager::UpdateAutoMC(const AircraftState& state_now,
                          const fixed fallback_mc)
{
  if (active_task &&
      active_task->UpdateAutoMC(glide_polar, state_now, fallback_mc))
    return true;

  if (!task_behaviour.auto_mc) 
    return false;

  if (task_behaviour.auto_mc_mode == TaskBehaviour::AUTOMC_FINALGLIDE)
    return false;

  if (positive(fallback_mc)) {
    glide_polar.SetMC(fallback_mc);
    return true;
  }

  return false;
}

GeoPoint
TaskManager::GetTaskCenter(const GeoPoint& fallback_location) const
{
  if (active_task)
    return active_task->GetTaskCenter(fallback_location);

  return fallback_location;
}

fixed
TaskManager::GetTaskRadius(const GeoPoint& fallback_location) const
{
  if (active_task)
    return active_task->GetTaskRadius(fallback_location);

  return fixed_zero;
}

const TCHAR*
TaskManager::GetOrderedTaskpointName(unsigned index) const
{
 static TCHAR buff[NAME_SIZE+1];
 buff[0] = '\0';

 if (!CheckOrderedTask())
   return buff;

 if (active_task == &task_ordered && index < TaskSize())
   CopyString(buff, task_ordered.GetTaskPoint(index)->GetWaypoint().name.c_str(),
              NAME_SIZE + 1);

 return buff;
}

bool
TaskManager::IsInSector (const unsigned index, const AircraftState &ref,
                         const bool AATOnly) const
{
  if (!CheckOrderedTask())
    return false;

  if (AATOnly) {
    const AATPoint *ap = task_ordered.GetAATTaskPoint(index);
    if (ap)
      return ap->IsInSector(ref);
  }
  else {
    const OrderedTaskPoint *p = task_ordered.GetTaskPoint(index);
    if (p)
      return p->IsInSector(ref);
  }

  return false;
}

const GeoPoint&
TaskManager::GetLocationTarget(const unsigned index,
                               const GeoPoint& fallback_location) const
{
  if (!CheckOrderedTask())
    return fallback_location;

  const AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    return ap->get_location_target();

 return fallback_location;
}
bool
TaskManager::TargetIsLocked(const unsigned index) const
{
  if (!CheckOrderedTask())
    return false;

  const AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    return ap->IsTargetLocked();

 return false;
}

bool
TaskManager::HasTarget(const unsigned index) const
{
  if (!CheckOrderedTask())
    return false;

  const AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    return ap->HasTarget();

 return false;
}

bool
TaskManager::SetTarget(const unsigned index, const GeoPoint &loc,
                       const bool override_lock)
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    ap->set_target(loc, override_lock);

  return true;
}

bool
TaskManager::SetTarget(const unsigned index, const fixed range,
                       const fixed radial)
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    ap->set_target(range, radial, task_ordered.GetTaskProjection());

  return true;
}

bool
TaskManager::GetTargetRangeRadial(const unsigned index, fixed &range,
                                  fixed &radial) const
{
  if (!CheckOrderedTask())
    return false;

  const AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    ap->get_target_range_radial(range, radial);

  return true;
}

bool
TaskManager::TargetLock(const unsigned index, bool do_lock)
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    ap->target_lock(do_lock);

  return true;
}

OrderedTask* 
TaskManager::Clone(TaskEvents &te, const TaskBehaviour &tb,
                   const GlidePolar &gp) const
{
  return task_ordered.Clone(te, tb, gp);
}

bool 
TaskManager::Commit(const OrderedTask& other)
{
  bool retval = task_ordered.Commit(other);

  if (other.TaskSize()) {
    // if valid, resume the task
    switch (mode) {
    case MODE_NULL:
      SetMode(MODE_ORDERED); // set mode first
      SetActiveTaskPoint(0); // then set tp
      break;
    case MODE_GOTO:
    case MODE_ABORT:
      // resume on load
      SetMode(MODE_ORDERED);
      break;
    case MODE_ORDERED:
      IncrementActiveTaskPoint(0); // ensure tp is within size
      break;
    };
  } else if (mode == MODE_ORDERED) {
    SetActiveTaskPoint(0); // reset tp of ordered task so will be zero
                           // on next load if valid
    SetMode(MODE_NULL);
  }

  return retval;
}

void
TaskManager::TakeoffAutotask(const GeoPoint& loc, const fixed terrain_alt)
{
  // create a goto task on takeoff
  if (!active_task && task_goto.TakeoffAutotask(loc, terrain_alt))
    SetMode(MODE_GOTO);
}
