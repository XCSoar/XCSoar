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

#include "TaskSolution.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "Navigation/Aircraft.hpp"
#include "Task/Points/TaskPoint.hpp"
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
