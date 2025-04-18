// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskManager.hpp"
#include "Ordered/OrderedTask.hpp"
#include "Ordered/Points/OrderedTaskPoint.hpp"
#include "Ordered/Points/AATPoint.hpp"
#include "Unordered/GotoTask.hpp"
#include "Unordered/AlternateTask.hpp"

TaskManager::TaskManager(const TaskBehaviour &_task_behaviour,
                         const Waypoints &wps) noexcept
  :task_behaviour(_task_behaviour),
   ordered_task(std::make_unique<OrderedTask>(task_behaviour)),
   goto_task(std::make_unique<GotoTask>(task_behaviour, wps)),
   abort_task(std::make_unique<AlternateTask>(task_behaviour, wps))
{
  null_stats.reset();
}

TaskManager::~TaskManager() noexcept = default;

void
TaskManager::SetTaskEvents(TaskEvents &_task_events) noexcept
{
  ordered_task->SetTaskEvents(_task_events);
  goto_task->SetTaskEvents(_task_events);
  abort_task->SetTaskEvents(_task_events);
}

void
TaskManager::SetTaskBehaviour(const TaskBehaviour &behaviour) noexcept
{
  task_behaviour = behaviour;

  safety_polar.SetMC(task_behaviour.safety_mc);

  ordered_task->SetTaskBehaviour(behaviour);
  goto_task->SetTaskBehaviour(behaviour);
  abort_task->SetTaskBehaviour(behaviour);
}

void
TaskManager::SetOrderedTaskSettings(const OrderedTaskSettings &otb) noexcept
{
  ordered_task->SetOrderedTaskSettings(otb);
}

TaskType
TaskManager::SetMode(const TaskType _mode) noexcept
{
  switch(_mode) {
  case TaskType::ABORT:
    active_task = abort_task.get();
    mode = TaskType::ABORT;
    break;

  case TaskType::ORDERED:
    if (ordered_task->TaskSize()) {
      active_task = ordered_task.get();
      mode = TaskType::ORDERED;
    }

    break;

  case TaskType::GOTO:
    if (goto_task->GetActiveTaskPoint()) {
      active_task = goto_task.get();
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
TaskManager::SetActiveTaskPoint(unsigned index) noexcept
{
  if (active_task)
    active_task->SetActiveTaskPoint(index);
}

unsigned
TaskManager::GetActiveTaskPointIndex() const noexcept
{
  if (active_task)
    return active_task->GetActiveTaskPointIndex();

  return 0;
}

void
TaskManager::IncrementActiveTaskPoint(int offset) noexcept
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

void
TaskManager::UpdateCommonStatsTimes(const AircraftState &state) noexcept
{
  if (ordered_task->TaskSize() > 1) {
    const TaskStats &task_stats = ordered_task->GetStats();

    common_stats.aat_time_remaining =
      ordered_task->GetOrderedTaskSettings().aat_min_time -
      task_stats.total.time_elapsed;

    const FloatDuration aat_time =
      ordered_task->GetOrderedTaskSettings().aat_min_time +
      task_behaviour.optimise_targets_margin;

    if (aat_time.count() > 0) {
      common_stats.aat_speed_max = task_stats.distance_max / aat_time.count();
      common_stats.aat_speed_min = task_stats.distance_min / aat_time.count();
      common_stats.aat_speed_target =
        task_stats.total.planned.GetDistance() / aat_time.count();
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
      if (!common_stats.TimeUnderStartMaxHeight.IsDefined() &&
          state.altitude < start_max_height) {
        common_stats.TimeUnderStartMaxHeight = state.time;
      }
      if (state.altitude > start_max_height) {
          common_stats.TimeUnderStartMaxHeight = TimeStamp::Undefined();
      }
    } else {
      common_stats.TimeUnderStartMaxHeight = TimeStamp::Undefined();
    }

    ordered_task->UpdateSummary(common_stats.ordered_summary);

  } else {
    common_stats.ResetTask();
  }
}

void
TaskManager::UpdateCommonStatsWaypoints(const AircraftState &state) noexcept
{
  common_stats.vector_home = state.location.IsValid()
    ? abort_task->GetHomeVector(state)
    : GeoVector::Invalid();

  common_stats.landable_reachable = abort_task->HasReachableLandable();
}

void
TaskManager::UpdateCommonStatsTask() noexcept
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
TaskManager::UpdateCommonStatsPolar(const AircraftState &state) noexcept
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
TaskManager::UpdateCommonStats(const AircraftState &state) noexcept
{
  UpdateCommonStatsTimes(state);
  UpdateCommonStatsTask();
  UpdateCommonStatsWaypoints(state);
  UpdateCommonStatsPolar(state);
}

bool
TaskManager::Update(const AircraftState &state,
                    const AircraftState &state_last) noexcept
{
  /* always update ordered task so even if we are temporarily in a
     different mode, so the task stats are still updated.  Otherwise,
     the task stats would freeze and sampling etc would not be
     performed.  In actual use, even if you are in Abort/Goto you
     still may want to go back to the task and have it know where you
     went with respect to your task turnpoints etc. */

  bool retval = false;

  if (state_last.time.IsDefined() && state.time.IsDefined() &&
      state_last.time > state.time)
    /* time warp */
    Reset();

  if (ordered_task->TaskSize() > 1) {
    // always update ordered task
    retval |= ordered_task->Update(state, state_last, glide_polar);
  }

  // inform the abort task whether it is running as the task or not
  abort_task->SetActive(active_task == abort_task.get());

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

  if (active_task && active_task != ordered_task.get() &&
      active_task != abort_task.get())
    // update mode task for any that have not yet run
    retval |= active_task->Update(state, state_last, glide_polar);

  UpdateCommonStats(state);

  return retval;
}

bool
TaskManager::UpdateIdle(const AircraftState &state) noexcept
{
  bool retval = false;

  if (active_task) {
    const GlidePolar &polar = active_task == abort_task.get()
      ? GetReachPolar()
      : glide_polar;

    retval |= active_task->UpdateIdle(state, polar);
  }

  return retval;
}

const TaskStats &
TaskManager::GetStats() const noexcept
{
  if (active_task)
    return active_task->GetStats();

  return null_stats;
}

bool
TaskManager::DoGoto(WaypointPtr &&wp) noexcept
{
  if (goto_task->DoGoto(std::move(wp))) {
    SetMode(TaskType::GOTO);
    return true;
  }

  return false;
}

bool
TaskManager::CheckTask() const noexcept
{
  if (active_task)
    return !IsError(active_task->CheckTask());

  return false;
}

bool
TaskManager::CheckOrderedTask() const noexcept
{
  return !IsError(ordered_task->CheckTask());
}

AbstractTaskFactory &
TaskManager::GetFactory() const noexcept
{
  return ordered_task->GetFactory();
}

void
TaskManager::SetFactory(const TaskFactoryType _factory) noexcept
{
  ordered_task->SetFactory(_factory);
}

TaskAdvance &
TaskManager::SetTaskAdvance() noexcept
{
  return ordered_task->SetTaskAdvance();
}

const AlternateList &
TaskManager::GetAlternates() const noexcept
{
  return abort_task->GetAlternates();
}

void
TaskManager::Reset() noexcept
{
  ordered_task->Reset();
  goto_task->Reset();
  abort_task->Reset();
  common_stats.Reset();
  glide_polar.SetCruiseEfficiency(1);
}

GeoPoint
TaskManager::RandomPointInTask(const unsigned index, const double mag) const noexcept
{
  if (active_task == ordered_task.get() && ordered_task->IsValidIndex(index))
    return ordered_task->GetTaskPoint(index).GetRandomPointInSector(mag);

  if (active_task && index <= active_task->TaskSize())
    return active_task->GetActiveTaskPoint()->GetLocation();

  return GeoPoint::Invalid();
}

void
TaskManager::SetGlidePolar(const GlidePolar &_glide_polar) noexcept
{
  glide_polar = _glide_polar;

  safety_polar = glide_polar;
  safety_polar.SetMC(task_behaviour.safety_mc);
}

bool
TaskManager::UpdateAutoMC(const AircraftState &state_now,
                          const double fallback_mc) noexcept
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
TaskManager::GetLocationTarget(const unsigned index) const noexcept
{
  const AATPoint *ap = ordered_task->GetAATTaskPoint(index);
  if (ap)
    return ap->GetTargetLocation();

  return GeoPoint::Invalid();
}
bool
TaskManager::TargetIsLocked(const unsigned index) const noexcept
{
  const AATPoint *ap = ordered_task->GetAATTaskPoint(index);
  if (ap)
    return ap->IsTargetLocked();

 return false;
}

bool
TaskManager::SetTarget(const unsigned index, const GeoPoint &loc,
                       const bool override_lock) noexcept
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = ordered_task->GetAATTaskPoint(index);
  if (ap)
    ap->SetTarget(loc, override_lock);

  return true;
}

bool
TaskManager::SetTarget(const unsigned index, RangeAndRadial rar) noexcept
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = ordered_task->GetAATTaskPoint(index);
  if (ap)
    ap->SetTarget(rar, ordered_task->GetTaskProjection());

  return true;
}

bool
TaskManager::TargetLock(const unsigned index, bool do_lock) noexcept
{
  if (!CheckOrderedTask())
    return false;

  AATPoint *ap = ordered_task->GetAATTaskPoint(index);
  if (ap)
    ap->LockTarget(do_lock);

  return true;
}

std::unique_ptr<OrderedTask>
TaskManager::Clone(const TaskBehaviour &tb) const noexcept
{
  return ordered_task->Clone(tb);
}

bool
TaskManager::Commit(const OrderedTask &other) noexcept
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
TaskManager::SetIntersectionTest(AbortIntersectionTest *test) noexcept
{
  abort_task->SetIntersectionTest(test);
}

void
TaskManager::TakeoffAutotask(const GeoPoint &loc, const double terrain_alt) noexcept
{
  // create a goto task on takeoff
  if (!active_task && goto_task->TakeoffAutotask(loc, terrain_alt))
    SetMode(TaskType::GOTO);
}

void
TaskManager::ResetTask() noexcept
{
  if (active_task) {
    active_task->Reset();
    UpdateCommonStatsTask();
  }
}
