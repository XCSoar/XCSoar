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

#include "TaskSolution.hpp"
#include "GlideSolvers/MacCready.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include "GlideSolvers/GlideState.hpp"
#include "Navigation/Aircraft.hpp"
#include "Task/Tasks/BaseTask/TaskPoint.hpp"

GlideResult 
TaskSolution::glide_solution_remaining(const TaskPoint& taskpoint,
                                       const AircraftState &ac, 
                                       const GlidePolar &polar,
                                       const fixed minH)
{
  GlideState gs(taskpoint.GetVectorRemaining(ac),
                max(minH,taskpoint.GetElevation()),
                ac.altitude, ac.wind);
  return MacCready::solve(polar, gs);
}

GlideResult 
TaskSolution::glide_solution_planned(const TaskPoint& taskpoint,
                                     const AircraftState &ac, 
                                     const GlidePolar &polar,
                                     const fixed minH)
{
  GlideState gs(taskpoint.GetVectorPlanned(),
                max(minH,taskpoint.GetElevation()),
                ac.altitude, ac.wind);
  return MacCready::solve(polar, gs);
}

GlideResult 
TaskSolution::glide_solution_travelled(const TaskPoint& taskpoint,
                                       const AircraftState &ac, 
                                       const GlidePolar &polar,
                                       const fixed minH)
{
  GlideState gs(taskpoint.GetVectorTravelled(ac),
                max(minH,taskpoint.GetElevation()),
                ac.altitude, ac.wind);
  return MacCready::solve(polar, gs);
}

GlideResult 
TaskSolution::glide_solution_sink(const TaskPoint& taskpoint,
                                  const AircraftState &ac, 
                                  const GlidePolar &polar,
                                  const fixed S)
{
  GlideState gs(taskpoint.GetVectorRemaining(ac),
                taskpoint.GetElevation(),
                ac.altitude, ac.wind);
  return MacCready::solve_sink(polar, gs, S);
}
