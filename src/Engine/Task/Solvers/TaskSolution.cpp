// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskSolution.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "Navigation/Aircraft.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Task/Points/TaskWaypoint.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"

#include <algorithm>

GlideResult
TaskSolution::GlideSolutionRemaining(const GeoPoint &location,
                                     const GeoPoint &target,
                                     const double target_elevation,
                                     const double altitude,
                                     const SpeedVector &wind,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar)
{
  assert(location.IsValid());
  assert(target.IsValid());

  GlideState gs(location.DistanceBearing(target),
                target_elevation, altitude, wind);

  return MacCready::Solve(settings, polar, gs);
}

GlideResult
TaskSolution::GlideSolutionRemaining(const TaskPoint &taskpoint,
                                     const AircraftState &ac,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar,
                                     const double min_h)
{
  /* A goto or abort point with no stored height must not be solved
     as 0 m MSL. */
  if (taskpoint.GetType() == TaskPointType::UNORDERED &&
      !static_cast<const TaskWaypoint &>(taskpoint).HasElevation()) {
    GlideResult res;
    res.Reset();
    return res;
  }

  const GlideState gs = GlideState::Remaining(taskpoint, ac, min_h);
  return MacCready::Solve(settings, polar, gs);
}

GlideResult
TaskSolution::GlideSolutionPlanned(const OrderedTaskPoint &taskpoint,
                                   const AircraftState &ac,
                                   const GlideSettings &settings,
                                   const GlidePolar &polar,
                                   const double min_h)
{
  assert(ac.location.IsValid());

  GlideState gs(taskpoint.GetVectorPlanned(),
                std::max(min_h, taskpoint.GetElevation()),
                ac.altitude, ac.wind);
  return MacCready::Solve(settings, polar, gs);
}

GlideResult
TaskSolution::GlideSolutionTravelled(const OrderedTaskPoint &taskpoint,
                                     const AircraftState &ac,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar,
                                     const double min_h)
{
  assert(ac.location.IsValid());

  GlideState gs(taskpoint.GetVectorTravelled(),
                std::max(min_h, taskpoint.GetElevation()),
                ac.altitude, ac.wind);
  return MacCready::Solve(settings, polar, gs);
}

GlideResult
TaskSolution::GlideSolutionSink(const TaskPoint &taskpoint,
                                const AircraftState &ac,
                                const GlideSettings &settings,
                                const GlidePolar &polar,
                                const double s)
{
  assert(ac.location.IsValid());

  GlideState gs(taskpoint.GetVectorRemaining(ac.location),
                taskpoint.GetElevation(),
                ac.altitude, ac.wind);
  return MacCready::SolveSink(settings, polar, gs, s);
}
