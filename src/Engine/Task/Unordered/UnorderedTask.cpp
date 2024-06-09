// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UnorderedTask.hpp"
#include "Task/Solvers/TaskBestMc.hpp"
#include "Task/Solvers/TaskGlideRequired.hpp"
#include "Task/Solvers/TaskSolution.hpp"
#include "Task/Points/TaskWaypoint.hpp"
#include "Navigation/Aircraft.hpp"

UnorderedTask::UnorderedTask(const TaskType _type,
                             const TaskBehaviour &tb):
  AbstractTask(_type, tb)
{
}

bool
UnorderedTask::CalcBestMC(const AircraftState &aircraft,
                          const GlidePolar &glide_polar,
                          double& best) const noexcept
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (tp == nullptr || !aircraft.location.IsValid()) {
    best = glide_polar.GetMC();
    return false;
  }

  TaskBestMc bmc(*tp, aircraft, task_behaviour.glide, glide_polar);
  return bmc.search(glide_polar.GetMC(), best);
}

TaskValidationErrorSet
UnorderedTask::CheckTask() const noexcept
{
  TaskValidationErrorSet errors;

  if (GetActiveTaskPoint() == nullptr)
    errors |= TaskValidationErrorType::EMPTY_TASK;

  return errors;
}

bool
UnorderedTask::CheckTransitions(const AircraftState &state_now,
                                [[maybe_unused]] const AircraftState &state_last) noexcept
{
  if (!stats.task_valid || !state_now.flying)
    return false;

  if (!stats.start.HasStarted()) {
    stats.start.SetStarted(state_now);
    return true;
  }

  return false;
}

double
UnorderedTask::CalcRequiredGlide(const AircraftState &aircraft,
                                 const GlidePolar &glide_polar) const noexcept
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (tp == nullptr || !aircraft.location.IsValid())
    return 0;

  TaskGlideRequired bgr(*tp, aircraft, task_behaviour.glide, glide_polar);
  return bgr.search(0);
}

void
UnorderedTask::GlideSolutionRemaining(const AircraftState &state,
                                      const GlidePolar &polar,
                                      GlideResult &total,
                                      GlideResult &leg) noexcept
{
  GlideResult res;

  TaskPoint* tp = GetActiveTaskPoint();
  if (tp != nullptr && state.location.IsValid()) {
    res = TaskSolution::GlideSolutionRemaining(*tp, state,
                                               task_behaviour.glide, polar);
    res.CalcDeferred();
  } else
    res.Reset();

  total = res;
  leg = res;
}

void
UnorderedTask::GlideSolutionTravelled([[maybe_unused]] const AircraftState &state,
                                      [[maybe_unused]] const GlidePolar &glide_polar,
                                      GlideResult &total,
                                      GlideResult &leg) noexcept
{
  GlideResult null_res;
  null_res.Reset();
  total = null_res;
  leg = null_res;
}

void
UnorderedTask::GlideSolutionPlanned([[maybe_unused]] const AircraftState &state,
                                    [[maybe_unused]] const GlidePolar &glide_polar,
                                    GlideResult &total,
                                    GlideResult &leg,
                                    DistanceStat &total_remaining_effective,
                                    DistanceStat &leg_remaining_effective,
                                    const GlideResult &solution_remaining_total,
                                    const GlideResult &solution_remaining_leg) noexcept
{
  total = solution_remaining_total;
  leg = solution_remaining_leg;

  if (total.IsOk())
    total_remaining_effective.SetDistance(total.vector.distance);
  else
    total_remaining_effective.Reset();

  if (leg.IsOk())
    leg_remaining_effective.SetDistance(leg.vector.distance);
  else
    leg_remaining_effective.Reset();
}

TimeStamp
UnorderedTask::ScanTotalStartTime() noexcept
{
  return TimeStamp::Undefined();
}

TimeStamp
UnorderedTask::ScanLegStartTime() noexcept
{
  return TimeStamp::Undefined();
}

void
UnorderedTask::ScanDistanceMinMax([[maybe_unused]] const GeoPoint &location, [[maybe_unused]] bool full,
                                  double *dmin, double *dmax) noexcept
{
  *dmin = *dmax = ScanDistanceNominal();
}

double 
UnorderedTask::ScanDistanceMaxTotal() noexcept
{
  return ScanDistanceNominal();
}

double
UnorderedTask::ScanDistanceNominal() const noexcept
{
  return stats.total.remaining.IsDefined()
    ? stats.total.remaining.GetDistance()
    : 0;
}

double
UnorderedTask::ScanDistancePlanned() noexcept
{
  return ScanDistanceNominal();
}

double
UnorderedTask::ScanDistanceScored([[maybe_unused]] const GeoPoint &location) noexcept
{
  return 0;
}

double
UnorderedTask::ScanDistanceTravelled([[maybe_unused]] const GeoPoint &location) noexcept
{
  return 0;
}

double
UnorderedTask::ScanDistanceRemaining(const GeoPoint &location) noexcept
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (tp == nullptr || !location.IsValid())
    return 0;

  return tp->Distance(location);
}

double
UnorderedTask::CalcGradient(const AircraftState &state) const noexcept
{
  return CalcLegGradient(state);
}
