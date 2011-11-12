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

#include "UnorderedTask.hpp"
#include "TaskSolvers/TaskBestMc.hpp"
#include "TaskSolvers/TaskGlideRequired.hpp"
#include "TaskSolvers/TaskSolution.hpp"

UnorderedTask::UnorderedTask(const enum Type _type, TaskEvents &te,
                             const TaskBehaviour &tb,
                             const GlidePolar &gp):
  AbstractTask(_type, te, tb, gp)
{
}


bool 
UnorderedTask::CalcBestMC(const AircraftState &aircraft, fixed& best) const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    best = glide_polar.GetMC();
    return false;
  }
  TaskBestMc bmc(tp, aircraft, glide_polar);
  return bmc.search(glide_polar.GetMC(), best);
}

bool
UnorderedTask::CheckTask() const
{
  return (GetActiveTaskPoint()!=NULL);
}

fixed 
UnorderedTask::CalcRequiredGlide(const AircraftState &aircraft) const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fixed_zero;
  }
  TaskGlideRequired bgr(tp, aircraft, glide_polar);
  return bgr.search(fixed_zero);
}

void
UnorderedTask::GlideSolutionRemaining(const AircraftState &state, 
                                        const GlidePolar &polar,
                                        GlideResult &total,
                                        GlideResult &leg)
{
  GlideResult res;

  TaskPoint* tp = GetActiveTaskPoint();
  if (tp) {
    res = TaskSolution::glide_solution_remaining(*tp, state, polar);
    res.CalcDeferred(state);
  } else
    res.Reset();

  total = res;
  leg = res;
}

void 
UnorderedTask::GlideSolutionTravelled(const AircraftState &state, 
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
    total_remaining_effective.set_distance(total.vector.distance);
  else
    total_remaining_effective.Reset();

  if (leg.IsOk())
    leg_remaining_effective.set_distance(leg.vector.distance);
  else
    leg_remaining_effective.Reset();
}



fixed 
UnorderedTask::ScanTotalStartTime(const AircraftState &state)
{
  return state.time;
}

fixed 
UnorderedTask::ScanLegStartTime(const AircraftState &state)
{
  return state.time;
}


void 
UnorderedTask::ScanDistanceMinMax(const GeoPoint &location, bool full,
                                    fixed *dmin, fixed *dmax)
{
  *dmin = *dmax = stats.total.remaining.IsDefined()
    ? stats.total.remaining.get_distance()
    : fixed_zero;
}

fixed 
UnorderedTask::ScanDistanceNominal()
{
  return stats.total.remaining.IsDefined()
    ? stats.total.remaining.get_distance()
    : fixed_zero;
}

fixed
UnorderedTask::ScanDistancePlanned()
{
  return stats.total.remaining.IsDefined()
    ? stats.total.remaining.get_distance()
    : fixed_zero;
}

fixed 
UnorderedTask::ScanDistanceScored(const GeoPoint &location)
{
  return fixed_zero;
}

fixed 
UnorderedTask::ScanDistanceTravelled(const GeoPoint &location)
{
  return fixed_zero;
}

fixed 
UnorderedTask::ScanDistanceRemaining(const GeoPoint &location)
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fixed_zero;
  }
  return tp->Distance(location);
}

fixed 
UnorderedTask::CalcGradient(const AircraftState &state) const
{
  return CalcLegGradient(state);
}

fixed
UnorderedTask::GetFinishHeight() const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fixed_zero;
  }
  return tp->GetElevation();
}

GeoPoint 
UnorderedTask::GetTaskCenter(const GeoPoint& fallback_location) const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fallback_location;
  } else {
    return tp->GetLocation().Interpolate(fallback_location, fixed_half);
  }
}

fixed 
UnorderedTask::GetTaskRadius(const GeoPoint& fallback_location) const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fixed_zero;
  } else {
    return half(tp->GetLocation().Distance(fallback_location));
  }
}

void 
UnorderedTask::AcceptStartPointVisitor(TaskPointConstVisitor& visitor, const bool reverse) const
{
  // do nothing!
}
