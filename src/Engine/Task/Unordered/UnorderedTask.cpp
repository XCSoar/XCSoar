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
                          double& best) const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (tp == nullptr || !aircraft.location.IsValid()) {
    best = glide_polar.GetMC();
    return false;
  }

  TaskBestMc bmc(tp, aircraft, task_behaviour.glide, glide_polar);
  return bmc.search(glide_polar.GetMC(), best);
}

bool
UnorderedTask::CheckTask() const
{
  return (GetActiveTaskPoint()!=NULL);
}

bool
UnorderedTask::CheckTransitions(const AircraftState &state_now,
                                const AircraftState &state_last)
{
  if (!stats.task_valid || !state_now.flying)
    return false;

  if (!stats.start.task_started) {
    stats.start.SetStarted(state_now);
    return true;
  }

  return false;
}

double
UnorderedTask::CalcRequiredGlide(const AircraftState &aircraft,
                                 const GlidePolar &glide_polar) const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (tp == nullptr || !aircraft.location.IsValid())
    return 0;

  TaskGlideRequired bgr(tp, aircraft, task_behaviour.glide, glide_polar);
  return bgr.search(0);
}

void
UnorderedTask::GlideSolutionRemaining(const AircraftState &state,
                                      const GlidePolar &polar,
                                      GlideResult &total,
                                      GlideResult &leg)
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
UnorderedTask::GlideSolutionTravelled(const AircraftState &state,
                                      const GlidePolar &glide_polar,
                                      GlideResult &total,
                                      GlideResult &leg)
{
  GlideResult null_res;
  null_res.Reset();
  total = null_res;
  leg = null_res;
}

void
UnorderedTask::GlideSolutionPlanned(const AircraftState &state,
                                    const GlidePolar &glide_polar,
                                    GlideResult &total,
                                    GlideResult &leg,
                                    DistanceStat &total_remaining_effective,
                                    DistanceStat &leg_remaining_effective,
                                    const GlideResult &solution_remaining_total,
                                    const GlideResult &solution_remaining_leg)
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

double
UnorderedTask::ScanTotalStartTime()
{
  return -1;
}

double
UnorderedTask::ScanLegStartTime()
{
  return -1;
}

void
UnorderedTask::ScanDistanceMinMax(const GeoPoint &location, bool full,
                                  double *dmin, double *dmax)
{
  *dmin = *dmax = stats.total.remaining.IsDefined()
    ? stats.total.remaining.GetDistance()
    : 0;
}

double
UnorderedTask::ScanDistanceNominal()
{
  return stats.total.remaining.IsDefined()
    ? stats.total.remaining.GetDistance()
    : 0;
}

double
UnorderedTask::ScanDistancePlanned()
{
  return stats.total.remaining.IsDefined()
    ? stats.total.remaining.GetDistance()
    : 0;
}

double
UnorderedTask::ScanDistanceScored(const GeoPoint &location)
{
  return 0;
}

double
UnorderedTask::ScanDistanceTravelled(const GeoPoint &location)
{
  return 0;
}

double
UnorderedTask::ScanDistanceRemaining(const GeoPoint &location)
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (tp == nullptr || !location.IsValid())
    return 0;

  return tp->Distance(location);
}

double
UnorderedTask::CalcGradient(const AircraftState &state) const
{
  return CalcLegGradient(state);
}
