/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Ordered/OrderedTask.hpp"
#include "Ordered/Points/OrderedTaskPoint.hpp"
#include "Ordered/Points/AATPoint.hpp"
#include "Unordered/GotoTask.hpp"
#include "Unordered/AlternateTask.hpp"

TaskManager::TaskManager(const TaskBehaviour &_task_behaviour,
                         const Waypoints &wps)
  :glide_polar(GlidePolar::Invalid()), safety_polar(GlidePolar::Invalid()),
   task_behaviour(_task_behaviour),
   ordered_task(new OrderedTask(task_behaviour)),
   goto_task(new GotoTask(task_behaviour, wps)),
   abort_task(new AlternateTask(task_behaviour, wps)),
   mode(TaskType::NONE),
   active_task(NULL) {
  null_stats.reset();
}

TaskManager::~TaskManager()
{
  delete abort_task;
  delete goto_task;
  delete ordered_task;
}

void
TaskManager::SetTaskEvents(TaskEvents &_task_events)
{
  ordered_task->SetTaskEvents(_task_events);
  goto_task->SetTaskEvents(_task_events);
  abort_task->SetTaskEvents(_task_events);
}

void
TaskManager::SetTaskBehaviour(const TaskBehaviour &behaviour)
{
  task_behaviour = behaviour;

  safety_polar.SetMC(task_behaviour.safety_mc);

  ordered_task->SetTaskBehaviour(behaviour);
  goto_task->SetTaskBehaviour(behaviour);
  abort_task->SetTaskBehaviour(behaviour);
}

void
TaskManager::SetOrderedTaskSettings(const OrderedTaskSettings &otb)
{
  ordered_task->SetOrderedTaskSettings(otb);
}

TaskType
TaskManager::SetMode(const TaskType _mode)
{
  switch(_mode) {
  case TaskType::ABORT:
    active_task = abort_task;
    mode = TaskType::ABORT;
    break;

  case TaskType::ORDERED:
    if (ordered_task->TaskSize()) {
      active_task = ordered_task;
      mode = TaskType::ORDERED;
    }

    break;

  case TaskType::GOTO:
    if (goto_task->GetActiveTaskPoint()) {
      active_task = goto_task;
      mode = TaskType::GOTO;
    }

    break;

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
        ordered_task->RotateOptionalStarts();
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
  if (ordered_task->TaskSize() > 1) {
    const TaskStats &task_stats = ordered_task->GetStats();

    common_stats.aat_time_remaining =
      ordered_task->GetOrderedTaskSettings().aat_min_time -
      task_stats.total.time_elapsed;

    auto aat_time = ordered_task->GetOrderedTaskSettings().aat_min_time +
      task_behaviour.optimise_targets_margin;

    if (aat_time > 0) {
      common_stats.aat_speed_max = task_stats.distance_max / aat_time;
      common_stats.aat_speed_min = task_stats.distance_min / aat_time;
      common_stats.aat_speed_target =
        task_stats.total.planned.GetDistance() / aat_time;
    } else {
      common_stats.aat_speed_max = -1;
      common_stats.aat_speed_min = -1;
      common_stats.aat_speed_target = -1;
    }

    const StartConstraints &start_constraints =
      ordered_task->GetOrderedTaskSettings().start_constraints;
    common_stats.start_open_time_span = start_constraints.open_time_span;
    const auto start_max_height =
      start_constraints.max_height +
      (start_constraints.max_height_ref == AltitudeReference::MSL
       ? 0
       : ordered_task->GetPoint(0).GetElevation());
    if (start_max_height > 0 &&
        state.location.IsValid() && state.flying) {
      if (common_stats.TimeUnderStartMaxHeight <= 0 &&
          state.altitude < start_max_height) {
        common_stats.TimeUnderStartMaxHeight = state.time;
      }
      if (state.altitude > start_max_height) {
          common_stats.TimeUnderStartMaxHeight = -1;
      }
    } else {
      common_stats.TimeUnderStartMaxHeight = -1;
    }

    ordered_task->UpdateSummary(common_stats.ordered_summary);

  } else {
    common_stats.ResetTask();
  }
}

void
TaskManager::UpdateCommonStatsWaypoints(const AircraftState &state)
{
  common_stats.vector_home = state.location.IsValid()
    ? abort_task->GetHomeVector(state)
    : GeoVector::Invalid();

  common_stats.landable_reachable = abort_task->HasReachableLandable();
}

void
TaskManager::UpdateCommonStatsTask()
{
  common_stats.task_type = mode;

  if (active_task && active_task->GetStats().task_valid) {
    common_stats.active_has_next = active_task->IsValidTaskPoint(1);
    common_stats.active_has_previous = active_task->IsValidTaskPoint(-1);
    common_stats.next_is_last = active_task->IsValidTaskPoint(1) &&
                                !active_task->IsValidTaskPoint(2);
    common_stats.previous_is_first = active_task->IsValidTaskPoint(-1) &&
                                     !active_task->IsValidTaskPoint(-2);
  } else {
    common_stats.active_has_next = false;
    common_stats.active_has_previous = false;
    common_stats.next_is_last = false;
    common_stats.previous_is_first = false;
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
  /* always update ordered task so even if we are temporarily in a
     different mode, so the task stats are still updated.  Otherwise,
     the task stats would freeze and sampling etc would not be
     performed.  In actual use, even if you are in Abort/Goto you
     still may want to go back to the task and have it know where you
     went with respect to your task turnpoints etc. */

  bool retval = false;

  if (state_last.time >= 0 && state.time >= 0 &&
      state_last.time > state.time)
    /* time warp */
    Reset();

  if (ordered_task->TaskSize() > 1) {
    // always update ordered task
    retval |= ordered_task->Update(state, state_last, glide_polar);
  }

  // inform the abort task whether it is running as the task or not
  abort_task->SetActive(active_task == abort_task);

  // and tell it where the task destination is (if any)
  const GeoPoint *destination = &state.location;

  if (task_behaviour.abort_task_mode == AbortTaskMode::HOME) {
    const auto home = abort_task->GetHome();
    if (home)
      destination = &home->location;
  } else if (task_behaviour.abort_task_mode == AbortTaskMode::TASK) {
    const OrderedTaskPoint *twp = (const OrderedTaskPoint *)
      ordered_task->GetActiveTaskPoint();
    if (twp)
      destination = &(twp->GetLocationRemaining());
  }

  abort_task->SetTaskDestination(*destination);

  retval |= abort_task->Update(state, state_last, GetReachPolar());

  if (active_task && active_task != ordered_task &&
      active_task != abort_task)
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
    const GlidePolar &polar = active_task == abort_task
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
TaskManager::DoGoto(WaypointPtr &&wp)
{
  if (goto_task->DoGoto(std::move(wp))) {
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

bool
TaskManager::CheckOrderedTask() const
{
  return ordered_task->CheckTask();
}

AbstractTaskFactory &
TaskManager::GetFactory() const
{
  return ordered_task->GetFactory();
}

void
TaskManager::SetFactory(const TaskFactoryType _factory)
{
  ordered_task->SetFactory(_factory);
}

TaskAdvance &
TaskManager::SetTaskAdvance()
{
  return ordered_task->SetTaskAdvance();
}

const AlternateList &
TaskManager::GetAlternates() const
{
  return abort_task->GetAlternates();
}

void
TaskManager::Reset()
{
  ordered_task->Reset();
  goto_task->Reset();
  abort_task->Reset();
  common_stats.Reset();
  glide_polar.SetCruiseEfficiency(1);
}

unsigned
TaskManager::TaskSize() const
{
  if (active_task)
    return active_task->TaskSize();

  return 0;
}

GeoPoint
TaskManager::RandomPointInTask(const unsigned index, const double mag) const
{
  if (active_task == ordered_task && ordered_task->IsValidIndex(index))
    return ordered_task->GetTaskPoint(index).GetRandomPointInSector(mag);

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

bool
TaskManager::UpdateAutoMC(const AircraftState &state_now,
                          const double fallback_mc)
{
  if (!state_now.location.IsValid())
    return false;

  if (active_task &&
      active_task->UpdateAutoMC(glide_polar, state_now, fallback_mc))
    return true;

  if (!task_behaviour.IsAutoMCCruiseEnabled())
    return false;

  if (fallback_mc > 0) {
    glide_polar.SetMC(fallback_mc);
    return true;
  }

  return false;
}

const GeoPoint
TaskManager::GetLocationTarget(const unsigned index) const
{
  const AATPoint *ap = ordered_task->GetAATTaskPoint(index);
  if (ap)
    return ap->GetTargetLocation();

  return GeoPoint::Invalid();
}
bool
TaskManager::TargetIsLocked(const unsigned index) const
{
  const AATPoint *ap = ordered_task->GetAATTaskPoint(index);
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

  AATPoint *ap = ordered_task->GetAATTaskPoint(index);
  if (ap)
    ap->SetTarget(loc, override_lock);

  return true;
}

bool
TaskManager::SetTarget(const unsigned index, RangeAndRadial rar)
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = ordered_task->GetAATTaskPoint(index);
  if (ap)
    ap->SetTarget(rar, ordered_task->GetTaskProjection());

  return true;
}

bool
TaskManager::TargetLock(const unsigned index, bool do_lock)
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = ordered_task->GetAATTaskPoint(index);
  if (ap)
    ap->LockTarget(do_lock);

  return true;
}

OrderedTask *
TaskManager::Clone(const TaskBehaviour &tb) const
{
  return ordered_task->Clone(tb);
}

bool
TaskManager::Commit(const OrderedTask &other)
{
  bool retval = ordered_task->Commit(other);

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
TaskManager::SetIntersectionTest(AbortIntersectionTest *test)
{
  abort_task->SetIntersectionTest(test);
}

void
TaskManager::TakeoffAutotask(const GeoPoint &loc, const double terrain_alt)
{
  // create a goto task on takeoff
  if (!active_task && goto_task->TakeoffAutotask(loc, terrain_alt))
    SetMode(TaskType::GOTO);
}

void
TaskManager::ResetTask()
{
  if (active_task != nullptr) {
    active_task->Reset();
    UpdateCommonStatsTask();
  }
}
