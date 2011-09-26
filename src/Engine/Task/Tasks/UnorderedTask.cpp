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
UnorderedTask::calc_mc_best(const AircraftState &aircraft, fixed& best) const
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
UnorderedTask::check_task() const
{
  return (GetActiveTaskPoint()!=NULL);
}

fixed 
UnorderedTask::calc_glide_required(const AircraftState &aircraft) const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fixed_zero;
  }
  TaskGlideRequired bgr(tp, aircraft, glide_polar);
  return bgr.search(fixed_zero);
}

void
UnorderedTask::glide_solution_remaining(const AircraftState &state, 
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
UnorderedTask::glide_solution_travelled(const AircraftState &state, 
                                        GlideResult &total,
                                        GlideResult &leg)
{
  GlideResult null_res;
  null_res.Reset();
  total = null_res;
  leg = null_res;
}

void 
UnorderedTask::glide_solution_planned(const AircraftState &state, 
                                      GlideResult &total,
                                      GlideResult &leg,
                                      DistanceStat &total_remaining_effective,
                                      DistanceStat &leg_remaining_effective,
                                      const fixed total_t_elapsed,
                                      const fixed leg_t_elapsed)
{
  const GlideResult &res = stats.total.solution_remaining;
  if (!res.IsDefined())
    return;

  total = res;
  leg = res;
  total_remaining_effective.set_distance(res.vector.distance);
  leg_remaining_effective.set_distance(res.vector.distance);
}



fixed 
UnorderedTask::scan_total_start_time(const AircraftState &state)
{
  return state.time;
}

fixed 
UnorderedTask::scan_leg_start_time(const AircraftState &state)
{
  return state.time;
}


void 
UnorderedTask::scan_distance_minmax(const GeoPoint &location, bool full,
                                    fixed *dmin, fixed *dmax)
{
  *dmin = stats.total.remaining.get_distance();
  *dmax = stats.total.remaining.get_distance();
}

fixed 
UnorderedTask::scan_distance_nominal()
{
  return fixed(stats.total.remaining.get_distance());
}

fixed
UnorderedTask::scan_distance_planned()
{
  return fixed(stats.total.remaining.get_distance());
}

fixed 
UnorderedTask::scan_distance_scored(const GeoPoint &location)
{
  return fixed_zero;
}

fixed 
UnorderedTask::scan_distance_travelled(const GeoPoint &location)
{
  return fixed_zero;
}

fixed 
UnorderedTask::scan_distance_remaining(const GeoPoint &location)
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fixed_zero;
  }
  return tp->Distance(location);
}

fixed 
UnorderedTask::calc_gradient(const AircraftState &state) const
{
  return leg_gradient(state);
}

fixed
UnorderedTask::get_finish_height() const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fixed_zero;
  }
  return tp->GetElevation();
}

GeoPoint 
UnorderedTask::get_task_center(const GeoPoint& fallback_location) const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fallback_location;
  } else {
    return tp->GetLocation().Interpolate(fallback_location, fixed_half);
  }
}

fixed 
UnorderedTask::get_task_radius(const GeoPoint& fallback_location) const
{
  TaskPoint *tp = GetActiveTaskPoint();
  if (!tp) {
    return fixed_zero;
  } else {
    return half(tp->GetLocation().Distance(fallback_location));
  }
}

void 
UnorderedTask::sp_CAccept(TaskPointConstVisitor& visitor, const bool reverse) const
{
  // do nothing!
}
