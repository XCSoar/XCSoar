/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Solvers/TaskSolution.hpp"
#include "Engine/Task/Unordered/UnorderedTaskPoint.hpp"
#include "Ordered/Points/OrderedTaskPoint.hpp"
#include "Ordered/Points/AATPoint.hpp"
#include "Util/StringUtil.hpp"

TaskManager::TaskManager(const TaskBehaviour &_task_behaviour,
                         const Waypoints &wps)
  :glide_polar(GlidePolar::Invalid()), safety_polar(GlidePolar::Invalid()),
   task_behaviour(_task_behaviour),
   task_ordered(task_behaviour),
   task_goto(task_behaviour, wps),
   task_abort(task_behaviour, wps),
   mode(TaskType::NONE),
   active_task(NULL) {
  null_stats.reset();
}

void
TaskManager::SetTaskBehaviour(const TaskBehaviour &behaviour)
{
  task_behaviour = behaviour;

  safety_polar.SetMC(task_behaviour.safety_mc);

  task_ordered.SetTaskBehaviour(behaviour);
  task_goto.SetTaskBehaviour(behaviour);
  task_abort.SetTaskBehaviour(behaviour);
}

TaskType
TaskManager::SetMode(const TaskType _mode)
{
  switch(_mode) {
  case TaskType::ABORT:
    active_task = &task_abort;
    mode = TaskType::ABORT;
    break;

  case TaskType::ORDERED:
    if (task_ordered.TaskSize()) {
      active_task = &task_ordered;
      mode = TaskType::ORDERED;
      break;
    }

  case TaskType::GOTO:
    if (task_goto.GetActiveTaskPoint()) {
      active_task = &task_goto;
      mode = TaskType::GOTO;
      break;
    }

  case TaskType::NONE:
    active_task = NULL;
    mode = TaskType::NONE;
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
      if (mode == TaskType::ORDERED)
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
    const TaskStats &task_stats = task_ordered.GetStats();

    common_stats.ordered_has_targets = task_ordered.HasTargets();

    common_stats.aat_time_remaining =
      task_ordered.GetOrderedTaskBehaviour().aat_min_time -
      task_stats.total.time_elapsed;

    if (task_stats.total.remaining.IsDefined() &&
        positive(common_stats.aat_time_remaining))
      common_stats.aat_speed_remaining =
          fixed(task_stats.total.remaining.GetDistance()) /
          common_stats.aat_time_remaining;
    else
      common_stats.aat_speed_remaining = fixed(-1);

    fixed aat_min_time = task_ordered.GetOrderedTaskBehaviour().aat_min_time;

    if (positive(aat_min_time)) {
      common_stats.aat_speed_max = task_stats.distance_max / aat_min_time;
      common_stats.aat_speed_min = task_stats.distance_min / aat_min_time;
    } else {
      common_stats.aat_speed_max = fixed(-1);
      common_stats.aat_speed_min = fixed(-1);
    }

    const StartConstraints &start_constraints =
      task_ordered.GetOrderedTaskBehaviour().start_constraints;
    common_stats.start_open_time_span = start_constraints.open_time_span;
    const fixed start_max_height =
      fixed(start_constraints.max_height) +
      (start_constraints.max_height_ref == AltitudeReference::MSL
       ? fixed(0)
       : task_ordered.GetPoint(0).GetElevation());
    if (positive(start_max_height) &&
        state.location.IsValid() && state.flying) {
      if (!positive(common_stats.TimeUnderStartMaxHeight) &&
          state.altitude < start_max_height) {
        common_stats.TimeUnderStartMaxHeight = state.time;
      }
      if (state.altitude > start_max_height) {
          common_stats.TimeUnderStartMaxHeight = fixed(-1);
      }
    } else {
      common_stats.TimeUnderStartMaxHeight = fixed(-1);
    }

    task_ordered.UpdateSummary(common_stats.ordered_summary);

  } else {
    common_stats.ResetTask();
  }
}

void
TaskManager::UpdateCommonStatsWaypoints(const AircraftState &state)
{
  common_stats.vector_home = state.location.IsValid()
    ? task_abort.GetHomeVector(state)
    : GeoVector::Invalid();

  common_stats.landable_reachable = task_abort.HasReachableLandable();
}

void
TaskManager::UpdateCommonStatsTask()
{
  common_stats.task_type = mode;

  common_stats.ordered_has_optional_starts =
    task_ordered.GetStats().task_valid && task_ordered.HasOptionalStarts();

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
  if (!state.location.IsValid() || !glide_polar.IsValid())
    return;

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
  UpdateCommonStatsTask();
  UpdateCommonStatsWaypoints(state);
  UpdateCommonStatsPolar(state);
}

bool
TaskManager::Update(const AircraftState &state,
                    const AircraftState &state_last)
{
  if (!state.location.IsValid()) {
    /* in case of GPS failure or and during startup (before the first
       GPS fix), update only the stats */
    UpdateCommonStats(state);
    return false;
  }

  /* always update ordered task so even if we are temporarily in a
     different mode, so the task stats are still updated.  Otherwise,
     the task stats would freeze and sampling etc would not be
     performed.  In actual use, even if you are in Abort/Goto you
     still may want to go back to the task and have it know where you
     went with respect to your task turnpoints etc. */

  bool retval = false;

  if (state_last.time > state.time)
    Reset();

  if (task_ordered.TaskSize() > 1) {
    if (IsMat())
      ScanInsertMatPoints(state, state_last);

    // always update ordered task
    retval |= task_ordered.Update(state, state_last, glide_polar);
  }

  // inform the abort task whether it is running as the task or not
  task_abort.SetActive(active_task == &task_abort);

  // and tell it where the task destination is (if any)
  const GeoPoint *destination = &state.location;

  if (task_behaviour.abort_task_mode == AbortTaskMode::HOME) {
    const Waypoint *home = task_abort.GetHome();
    if (home)
      destination = &home->location;
  } else if (task_behaviour.abort_task_mode == AbortTaskMode::TASK) {
    const OrderedTaskPoint *twp = (const OrderedTaskPoint *)
      task_ordered.GetActiveTaskPoint();
    if (twp)
      destination = &(twp->GetLocationRemaining());
  }

  task_abort.SetTaskDestination(*destination);

  retval |= task_abort.Update(state, state_last, GetReachPolar());

  if (active_task && active_task != &task_ordered &&
      active_task != &task_abort)
    // update mode task for any that have not yet run
    retval |= active_task->Update(state, state_last, glide_polar);

  UpdateCommonStats(state);

  return retval;
}

bool
TaskManager::UpdateIdle(const AircraftState &state)
{
  bool retval = false;

  if (active_task) {
    const GlidePolar &polar = active_task == &task_abort
      ? GetReachPolar()
      : glide_polar;

    retval |= active_task->UpdateIdle(state, polar);
  }

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
TaskManager::ScanInsertMatPoints(const AircraftState &state,
                                 const AircraftState &state_last)
{
  assert(IsMat());

  //flat boxes 3600m (~2.2 miles/ side) filters out most points
  Angle angle_ur = Angle::FullCircle() / 8;
  Angle angle_ll = (Angle::FullCircle() * 5) / 8;

  GeoVector vect_ur(fixed(1800), angle_ur);
  GeoVector vect_ll(fixed(1800), angle_ll);

  GeoPoint last_ur = vect_ur.EndPoint(state_last.location);
  GeoPoint last_ll = vect_ll.EndPoint(state_last.location);

  GeoPoint now_ur = vect_ur.EndPoint(state.location);
  GeoPoint now_ll = vect_ll.EndPoint(state.location);

  const TaskProjection& task_projection = task_ordered.GetTaskProjection();
  FlatBoundingBox bb_last(task_projection.ProjectInteger(last_ll),
                          task_projection.ProjectInteger(last_ur));
  FlatBoundingBox bb_now(task_projection.ProjectInteger(now_ll),
                         task_projection.ProjectInteger(now_ur));

  const unsigned last_achieved_index = task_ordered.GetLastIntermediateAchieved();

  for (auto *i : task_ordered.GetMatPoints()) {
    OrderedTaskPoint &mat_tp = *i;
    if (task_ordered.CheckTransitionPointMat(mat_tp, state, state_last,
                                              bb_now, bb_last)) {
      if (!task_ordered.ShouldAddToMat(mat_tp.GetWaypoint()))
        break;

      OrderedTask *new_task = task_ordered.Clone(task_behaviour);
      new_task->Insert(mat_tp, last_achieved_index + 1);

      /* kludge: preserve the MatPoints */
      new_task->SetMatPoints() = task_ordered.GetMatPoints();
      task_ordered.SetMatPoints().clear();

      Commit(*new_task);

      SetActiveTaskPoint(last_achieved_index + 1);
      return true;
    }
  }
  return false;
}

bool
TaskManager::DoGoto(const Waypoint &wp)
{
  if (task_goto.DoGoto(wp)) {
    SetMode(TaskType::GOTO);
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
  common_stats.Reset();
  glide_polar.SetCruiseEfficiency(fixed(1));
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
  if (active_task == &task_ordered && task_ordered.IsValidIndex(index))
    return task_ordered.GetTaskPoint(index).GetRandomPointInSector(mag);

  if (active_task != NULL && index <= active_task->TaskSize())
    return active_task->GetActiveTaskPoint()->GetLocation();

  return GeoPoint::Invalid();
}

void
TaskManager::SetGlidePolar(const GlidePolar &_glide_polar)
{
  glide_polar = _glide_polar;

  safety_polar = glide_polar;
  safety_polar.SetMC(task_behaviour.safety_mc);
}

fixed
TaskManager::GetFinishHeight() const
{
  if (active_task)
    return active_task->GetFinishHeight();

  return fixed(0);
}

bool
TaskManager::UpdateAutoMC(const AircraftState &state_now,
                          const fixed fallback_mc)
{
  if (!state_now.location.IsValid())
    return false;

  if (active_task &&
      active_task->UpdateAutoMC(glide_polar, state_now, fallback_mc))
    return true;

  if (!task_behaviour.IsAutoMCCruiseEnabled())
    return false;

  if (positive(fallback_mc)) {
    glide_polar.SetMC(fallback_mc);
    return true;
  }

  return false;
}

const GeoPoint
TaskManager::GetLocationTarget(const unsigned index) const
{
  const AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    return ap->GetTargetLocation();

  return GeoPoint::Invalid();
}
bool
TaskManager::TargetIsLocked(const unsigned index) const
{
  const AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    return ap->IsTargetLocked();

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
    ap->SetTarget(loc, override_lock);

  return true;
}

bool
TaskManager::SetTarget(const unsigned index, RangeAndRadial rar)
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    ap->SetTarget(rar, task_ordered.GetTaskProjection());

  return true;
}

bool
TaskManager::TargetLock(const unsigned index, bool do_lock)
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = task_ordered.GetAATTaskPoint(index);
  if (ap)
    ap->LockTarget(do_lock);

  return true;
}

OrderedTask *
TaskManager::Clone(const TaskBehaviour &tb) const
{
  return task_ordered.Clone(tb);
}

bool
TaskManager::Commit(const OrderedTask &other)
{
  bool retval = task_ordered.Commit(other);

  if (other.TaskSize()) {
    // if valid, resume the task
    switch (mode) {
    case TaskType::NONE:
      SetMode(TaskType::ORDERED); // set mode first
      SetActiveTaskPoint(0); // then set tp
      break;
    case TaskType::GOTO:
    case TaskType::ABORT:
      // resume on load
      SetMode(TaskType::ORDERED);
      break;
    case TaskType::ORDERED:
      IncrementActiveTaskPoint(0); // ensure tp is within size
      break;
    };
  } else if (mode == TaskType::ORDERED) {
    SetActiveTaskPoint(0); // reset tp of ordered task so will be zero
                           // on next load if valid
    SetMode(TaskType::NONE);
  }

  return retval;
}

void
TaskManager::TakeoffAutotask(const GeoPoint &loc, const fixed terrain_alt)
{
  // create a goto task on takeoff
  if (!active_task && task_goto.TakeoffAutotask(loc, terrain_alt))
    SetMode(TaskType::GOTO);
}
