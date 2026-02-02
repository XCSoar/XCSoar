// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskMacCreadyTravelled.hpp"
#include "TaskSolution.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Navigation/Aircraft.hpp"

GlideResult
TaskMacCreadyTravelled::SolvePoint(const TaskPoint &tp,
                                   const AircraftState &aircraft,
                                   double minH) const
{
  assert(tp.GetType() != TaskPointType::UNORDERED);
  const OrderedTaskPoint &otp = (const OrderedTaskPoint &)tp;

  return TaskSolution::GlideSolutionTravelled(otp, aircraft,
                                              settings, glide_polar, minH);
}

AircraftState
TaskMacCreadyTravelled::get_aircraft_start(const AircraftState &aircraft) const
{
  const OrderedTaskPoint &tp = *(const OrderedTaskPoint *)points[0];
  assert(tp.GetType() != TaskPointType::UNORDERED);

  if (tp.HasEntered()) {
    const AircraftState &exited = tp.GetExitedState();
    // Check if the exited state has a valid location
    if (exited.location.IsValid()) {
      return exited;
    }
  }
  return aircraft;
}

double
TaskMacCreadyTravelled::get_min_height(const AircraftState &aircraft) const
{
  return aircraft.altitude;
}
