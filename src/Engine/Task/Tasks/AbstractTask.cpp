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

#include "AbstractTask.hpp"
#include "Navigation/Aircraft.hpp"
#include "BaseTask/TaskWaypoint.hpp"
#include "Util/Gradient.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/TaskEvents.hpp"
#include "Task/TaskBehaviour.hpp"

AbstractTask::AbstractTask(enum Type _type, TaskEvents &te,
                           const TaskBehaviour &tb, const GlidePolar &gp)
  :TaskInterface(_type),
   active_task_point(0),
   active_task_point_last(0-1),
   task_events(te),
   task_behaviour(tb),
   glide_polar(gp),
   mc_lpf(fixed(8)),
   ce_lpf(fixed(60)),
   em_lpf(fixed(60)),
   trigger_auto(false)
{
   stats.reset();
}

bool 
AbstractTask::UpdateAutoMC(GlidePolar &glide_polar,
                           const AircraftState& state, fixed fallback_mc)
{
  if (!positive(fallback_mc))
    fallback_mc = glide_polar.GetMC();

  if (!TaskStarted(true) || !task_behaviour.auto_mc) {
    ResetAutoMC();
    return false;
  }

  if (task_behaviour.auto_mc_mode == TaskBehaviour::AUTOMC_CLIMBAVERAGE) {
    stats.mc_best = mc_lpf.reset(fallback_mc);
    trigger_auto = false;
    return false;
  }

  fixed mc_found;
  if (CalcBestMC(state, mc_found)) {
    // improved solution found, activate auto fg mode
    if (mc_found > stats.mc_best)
      trigger_auto = true;
  } else {
    // no solution even at mc=0, deactivate auto fg mode
    trigger_auto = false;
  }

  if (trigger_auto) {
    // smooth out updates
    stats.mc_best = mc_lpf.update(mc_found);
    glide_polar.SetMC(stats.mc_best);
  } else {
    // reset lpf so will be smooth next time it becomes active
    stats.mc_best = mc_lpf.reset(fallback_mc);
  }

  return trigger_auto;
}

bool 
AbstractTask::UpdateIdle(const AircraftState &state)
{
  if (TaskStarted() && task_behaviour.calc_cruise_efficiency) {
    fixed val = fixed_one;
    if (CalcCruiseEfficiency(state, val))
      stats.cruise_efficiency = ce_lpf.update(val);
  } else {
    stats.cruise_efficiency = ce_lpf.reset(fixed_one);
  }

  if (TaskStarted() && task_behaviour.calc_effective_mc) {
    fixed val = glide_polar.GetMC();
    if (CalcEffectiveMC(state, val))
      stats.effective_mc = em_lpf.update(val);
  } else {
    stats.effective_mc = em_lpf.reset(glide_polar.GetMC());
  }

  if (task_behaviour.calc_glide_required)
    UpdateStatsGlide(state);
  else
    stats.glide_required = fixed_zero; // error

  return false;
}

unsigned 
AbstractTask::GetActiveTaskPointIndex() const
{
  return active_task_point;
}

void
AbstractTask::UpdateStatsDistances(const GeoPoint &location,
                                   const bool full_update)
{
  const TaskPoint *active = GetActiveTaskPoint();
  stats.current_leg.vector_remaining = active != NULL
    ? active->GetVectorRemaining(location)
    : GeoVector::Invalid();

  stats.total.remaining.set_distance(ScanDistanceRemaining(location));

  if (full_update)
    stats.distance_nominal = ScanDistanceNominal();

  ScanDistanceMinMax(location, full_update,
                       &stats.distance_min, &stats.distance_max);

  stats.total.travelled.set_distance(ScanDistanceTravelled(location));
  stats.total.planned.set_distance(ScanDistancePlanned());

  if (IsScored()) {
    if (!TaskStarted()) 
      stats.distance_scored = fixed_zero;
    else if (!TaskFinished()) 
      stats.distance_scored = ScanDistanceScored(location);
  } else
    stats.distance_scored = fixed_zero;
}

static void
Copy(DistanceStat &stat, const GlideResult &solution)
{
  if (solution.IsDefined())
    stat.set_distance(solution.vector.distance);
  else
    stat.Reset();
}

static void
CalculatePirker(DistanceStat &pirker, const DistanceStat &planned,
                const DistanceStat &remaining_effective)
{
  if (planned.IsDefined() && remaining_effective.IsDefined())
    pirker.set_distance(planned.get_distance() -
                        remaining_effective.get_distance());
  else
    pirker.Reset();
}

void
AbstractTask::UpdateGlideSolutions(const AircraftState &state)
{
  GlideSolutionRemaining(state, glide_polar, stats.total.solution_remaining,
                           stats.current_leg.solution_remaining);

  if (positive(glide_polar.GetMC())) {
    GlidePolar polar_mc0 = glide_polar;
    polar_mc0.SetMC(fixed_zero); 
    
    GlideSolutionRemaining(state, polar_mc0, stats.total.solution_mc0,
                             stats.current_leg.solution_mc0);
  } else {
    // no need to re-calculate, just copy
    stats.total.solution_mc0 = stats.total.solution_remaining;
    stats.current_leg.solution_mc0 = stats.current_leg.solution_remaining;
  }

  GlideSolutionTravelled(state, stats.total.solution_travelled,
                           stats.current_leg.solution_travelled);

  GlideSolutionPlanned(state, stats.total.solution_planned,
                         stats.current_leg.solution_planned,
                         stats.total.remaining_effective,
                         stats.current_leg.remaining_effective,
                         stats.total.solution_remaining,
                         stats.current_leg.solution_remaining);

  CalculatePirker(stats.total.pirker, stats.total.planned,
                  stats.total.remaining_effective);

  CalculatePirker(stats.current_leg.pirker, stats.current_leg.planned,
                  stats.current_leg.remaining_effective);

  Copy(stats.current_leg.remaining, stats.current_leg.solution_remaining);
  Copy(stats.current_leg.travelled, stats.current_leg.solution_travelled);
  Copy(stats.current_leg.planned, stats.current_leg.solution_planned);

  stats.total.gradient = ::AngleToGradient(CalcGradient(state));
  stats.current_leg.gradient = ::AngleToGradient(CalcLegGradient(state));
}

bool
AbstractTask::Update(const AircraftState &state, 
                     const AircraftState &state_last)
{
  stats.task_valid = CheckTask();
  stats.has_targets = HasTargets();

  const bool full_update = 
    CheckTransitions(state, state_last) ||
    (active_task_point != active_task_point_last);

  UpdateStatsTimes(state);
  UpdateStatsDistances(state.location, full_update);
  UpdateGlideSolutions(state);
  bool sample_updated = UpdateSample(state, full_update);
  UpdateStatsSpeeds(state, state_last);
  UpdateFlightMode();

  active_task_point_last = active_task_point;

  return sample_updated || full_update;
}

void
AbstractTask::UpdateStatsSpeeds(const AircraftState &state, 
                                const AircraftState &state_last)
{
  if (!TaskFinished()) {
    if (TaskStarted()) {
      const fixed dt = state.time - state_last.time;
      stats_computer.total.CalcSpeeds(stats.total, dt);
      stats_computer.current_leg.CalcSpeeds(stats.current_leg, dt);
    } else {
      stats_computer.total.Reset(stats.total);
      stats_computer.current_leg.Reset(stats.current_leg);
    }
  }
}

void
AbstractTask::UpdateStatsGlide(const AircraftState &state)
{
  stats.glide_required = AngleToGradient(CalcRequiredGlide(state));
}

void
AbstractTask::UpdateStatsTimes(const AircraftState &state)
{
  // default for tasks with no start time...
  stats.Time = state.time;
  if (!TaskFinished()) {
    stats.total.SetTimes(ScanTotalStartTime(state), state);
    stats.current_leg.SetTimes(ScanLegStartTime(state),state);
  }
}

void
AbstractTask::ResetAutoMC()
{
  stats.mc_best = mc_lpf.reset(glide_polar.GetMC());
  trigger_auto = false;
}

void 
AbstractTask::Reset()
{
  ResetAutoMC();
  active_task_point_last = 0 - 1;
  ce_lpf.reset(fixed_one);
  stats.reset();
}

fixed
AbstractTask::CalcLegGradient(const AircraftState &aircraft) const
{
  // Get next turnpoint
  const TaskWaypoint *tp = GetActiveTaskPoint();
  if (!tp)
    return fixed_zero;

  // Get the distance to the next turnpoint
  const fixed d = tp->GetVectorRemaining(aircraft.location).distance;
  if (!d)
    return fixed_zero;

  // Calculate the geometric gradient (height divided by distance)
  return (aircraft.altitude - tp->GetElevation()) / d;
}

bool 
AbstractTask::CalcEffectiveMC(const AircraftState &state_now, fixed& val) const
{
  val = glide_polar.GetMC();
  return true;
}

void
AbstractTask::UpdateFlightMode()
{
  if (!stats.calc_flight_mode())
    return;

  task_events.FlightModeTransition(stats.flight_mode_final_glide);
}
