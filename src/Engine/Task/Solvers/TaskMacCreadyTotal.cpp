// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskMacCreadyTotal.hpp"
#include "TaskSolution.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"

GlideResult
TaskMacCreadyTotal::SolvePoint(const TaskPoint &tp,
                               const AircraftState &aircraft,
                               double minH) const
{
  assert(tp.GetType() != TaskPointType::UNORDERED);
  const OrderedTaskPoint &otp = (const OrderedTaskPoint &)tp;

  return TaskSolution::GlideSolutionPlanned(otp, aircraft,
                                            settings, glide_polar, minH);
}

AircraftState
TaskMacCreadyTotal::get_aircraft_start(const AircraftState &aircraft) const
{
  const OrderedTaskPoint &tp = *(const OrderedTaskPoint *)points[0];
  assert(tp.GetType() != TaskPointType::UNORDERED);

  if (tp.HasEntered()) {
    return tp.GetScoredState();
  } else if (aircraft.location.IsValid()) {
    return aircraft;
  } else {
    /* fall back to actual start location */
    AircraftState aircraft2 = aircraft;
    aircraft2.location = tp.GetLocation();
    return aircraft2;
  }
}

double
TaskMacCreadyTotal::effective_distance(const FloatDuration time_remaining) const noexcept
{
  FloatDuration t_total{};
  double d_total = 0;
  for (int i = points.size() - 1; i >= 0; i--) {
    const GlideResult &result = leg_solutions[i];

    if (result.IsOk() && result.time_elapsed.count() > 0) {
      auto p = (time_remaining - t_total) / result.time_elapsed;
      if (p >= 0 && p <= 1) {
        return d_total + p * result.vector.distance;
      }

      d_total += result.vector.distance;
      t_total += result.time_elapsed;
    }
  }
  return d_total;
}

double
TaskMacCreadyTotal::effective_leg_distance(const FloatDuration time_remaining) const noexcept
{
  const GlideResult &result = get_active_solution();
  if (result.time_elapsed.count() <= 0)
    /* this can happen if the distance is zero; prevent division by
       zero by checking for this special case */
    return 0;

  auto p = time_remaining / result.time_elapsed;
  return p * result.vector.distance;
}

