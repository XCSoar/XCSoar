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

#include "TaskSolution.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "Navigation/Aircraft.hpp"
#include "Task/Points/TaskPoint.hpp"

GlideResult 
TaskSolution::GlideSolutionRemaining(const GeoPoint &location,
                                     const GeoPoint &target,
                                     const fixed target_elevation,
                                     const fixed altitude,
                                     const SpeedVector &wind,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar)
{
  GlideState gs(location.DistanceBearing(target),
                target_elevation, altitude, wind);

  return MacCready::Solve(settings, polar, gs);
}

GlideResult
TaskSolution::GlideSolutionRemaining(const TaskPoint &taskpoint,
                                     const AircraftState &ac,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar,
                                     const fixed min_h)
{
  GlideState gs(taskpoint.GetVectorRemaining(ac.location),
                std::max(min_h, taskpoint.GetElevation()),
                ac.altitude, ac.wind);
  return MacCready::Solve(settings, polar, gs);
}

GlideResult 
TaskSolution::GlideSolutionPlanned(const TaskPoint &taskpoint,
                                   const AircraftState &ac,
                                   const GlideSettings &settings,
                                   const GlidePolar &polar,
                                   const fixed min_h)
{
  GlideState gs(taskpoint.GetVectorPlanned(),
                std::max(min_h, taskpoint.GetElevation()),
                ac.altitude, ac.wind);
  return MacCready::Solve(settings, polar, gs);
}

GlideResult 
TaskSolution::GlideSolutionTravelled(const TaskPoint &taskpoint,
                                     const AircraftState &ac,
                                     const GlideSettings &settings,
                                     const GlidePolar &polar,
                                     const fixed min_h)
{
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
                                const fixed s)
{
  GlideState gs(taskpoint.GetVectorRemaining(ac.location),
                taskpoint.GetElevation(),
                ac.altitude, ac.wind);
  return MacCready::SolveSink(settings, polar, gs, s);
}
