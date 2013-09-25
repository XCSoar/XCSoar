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

#include "AbstractTask.hpp"
#include "Navigation/Aircraft.hpp"
#include "Points/TaskWaypoint.hpp"
#include "Util/Gradient.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/TaskBehaviour.hpp"

AbstractTask::AbstractTask(TaskType _type,
                           const TaskBehaviour &tb)
  :TaskInterface(_type),
   active_task_point(0),
   task_events(NULL),
   task_behaviour(tb),
   force_full_update(true),
   mc_lpf(fixed(8)),
   ce_lpf(fixed(60)),
   em_lpf(fixed(60)),
   mc_lpf_valid(false)
{
   stats.reset();
}

bool 
AbstractTask::UpdateAutoMC(GlidePolar &glide_polar,
                           const AircraftState& state, fixed fallback_mc)
{
  if (!task_behaviour.auto_mc) {
    /* AutoMC disabled in configuration */
    ResetAutoMC();
    return false;
  }

  fixed mc_found;
  if (task_behaviour.IsAutoMCFinalGlideEnabled() &&
      TaskStarted(true) && stats.flight_mode_final_glide) {
    /* calculate final glide MacCready */

    if (CalcBestMC(state, glide_polar, mc_found)) {
      /* final glide MacCready found */
      if (mc_lpf_valid)
        stats.mc_best = std::max(mc_lpf.Update(mc_found), fixed(0));
      else {
        stats.mc_best = std::max(mc_lpf.Reset(mc_found), fixed(0));
        mc_lpf_valid = true;
      }
    } else
      /* below final glide, but above margin */
      stats.mc_best = fixed(0);

    glide_polar.SetMC(stats.mc_best);
    return true;
  } else if (task_behaviour.IsAutoMCCruiseEnabled()) {
    /* cruise: set MacCready to recent climb average */

    if (!positive(fallback_mc)) {
      /* no climb average calculated yet */
      ResetAutoMC();
      return false;
    }

    stats.mc_best = fallback_mc;
    glide_polar.SetMC(stats.mc_best);
    return true;
  } else if (TaskStarted(true)) {
    /* no solution, but forced final glide AutoMacCready - converge to
       zero */

    mc_found = fixed(0);
    if (mc_lpf_valid)
      stats.mc_best = std::max(mc_lpf.Update(mc_found), fixed(0));
    else {
      stats.mc_best = std::max(mc_lpf.Reset(mc_found), fixed(0));
      mc_lpf_valid = true;
    }

    glide_polar.SetMC(stats.mc_best);
    return true;
  } else {
    ResetAutoMC();
    return false;
  }
}

bool 
AbstractTask::UpdateIdle(const AircraftState &state,
                         const GlidePolar &glide_polar)
{
  const bool valid = state.location.IsValid() && glide_polar.IsValid();

  if (stats.start.task_started && task_behaviour.calc_cruise_efficiency &&
      valid) {
    fixed val = fixed(1);
    if (CalcCruiseEfficiency(state, glide_polar, val))
      stats.cruise_efficiency = std::max(ce_lpf.Update(val), fixed(0));
  } else {
    stats.cruise_efficiency = ce_lpf.Reset(fixed(1));
  }

  if (stats.start.task_started && task_behaviour.calc_effective_mc &&
      valid) {
    fixed val = glide_polar.GetMC();
    if (CalcEffectiveMC(state, glide_polar, val))
      stats.effective_mc = std::max(em_lpf.Update(val), fixed(0));
  } else {
    stats.effective_mc = em_lpf.Reset(glide_polar.GetMC());
  }

  if (task_behaviour.calc_glide_required && valid)
    UpdateStatsGlide(state, glide_polar);
  else
    stats.glide_required = fixed(0); // error

  return false;
}

void
AbstractTask::UpdateStatsDistances(const GeoPoint &location,
                                   const bool full_update)
{
  stats.total.remaining.SetDistance(ScanDistanceRemaining(location));

  const TaskPoint *active = GetActiveTaskPoint();
  if (active != NULL) {
    stats.current_leg.location_remaining = active->GetLocationRemaining();
    stats.current_leg.vector_remaining = active->GetVectorRemaining(location);
    stats.current_leg.next_leg_vector = active->GetNextLegVector();
  } else {
    stats.current_leg.location_remaining = GeoPoint::Invalid();
    stats.current_leg.vector_remaining = GeoVector::Invalid();
    stats.current_leg.next_leg_vector = GeoVector::Invalid();
  }

  if (full_update)
    stats.distance_nominal = ScanDistanceNominal();

  ScanDistanceMinMax(location, full_update,
                       &stats.distance_min, &stats.distance_max);

  stats.total.travelled.SetDistance(ScanDistanceTravelled(location));
  stats.total.planned.SetDistance(ScanDistancePlanned());

  if (IsScored()) {
    if (!stats.start.task_started)
      stats.distance_scored = fixed(0);
    else if (!stats.task_finished)
      stats.distance_scored = ScanDistanceScored(location);
  } else
    stats.distance_scored = fixed(0);
}

static void
Copy(DistanceStat &stat, const GlideResult &solution)
{
  if (solution.IsDefined())
    stat.SetDistance(solution.vector.distance);
  else
    stat.Reset();
}

static void
CalculatePirker(DistanceStat &pirker, const DistanceStat &planned,
                const DistanceStat &remaining_effective)
{
  if (planned.IsDefined() && remaining_effective.IsDefined())
    pirker.SetDistance(planned.GetDistance() -
                        remaining_effective.GetDistance());
  else
    pirker.Reset();
}

void
AbstractTask::UpdateGlideSolutions(const AircraftState &state,
                                   const GlidePolar &glide_polar)
{
  GlideSolutionRemaining(state, glide_polar, stats.total.solution_remaining,
                           stats.current_leg.solution_remaining);

  if (positive(glide_polar.GetMC())) {
    GlidePolar polar_mc0 = glide_polar;
    polar_mc0.SetMC(fixed(0)); 
    
    GlideSolutionRemaining(state, polar_mc0, stats.total.solution_mc0,
                             stats.current_leg.solution_mc0);
  } else {
    // no need to re-calculate, just copy
    stats.total.solution_mc0 = stats.total.solution_remaining;
    stats.current_leg.solution_mc0 = stats.current_leg.solution_remaining;
  }

  GlideSolutionTravelled(state, glide_polar,
                         stats.total.solution_travelled,
                           stats.current_leg.solution_travelled);

  GlideSolutionPlanned(state, glide_polar,
                       stats.total.solution_planned,
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
                     const AircraftState &state_last,
                     const GlidePolar &glide_polar)
{
  stats.active_index = GetActiveTaskPointIndex();
  stats.task_valid = CheckTask();

  const bool full_update = 
    (state.location.IsValid() && state_last.location.IsValid() &&
     CheckTransitions(state, state_last)) ||
    force_full_update;
  force_full_update = false;

  UpdateStatsDistances(state.location, full_update);
  UpdateGlideSolutions(state, glide_polar);
  UpdateStatsTimes(state.time);

  const bool sample_updated = state.location.IsValid() &&
    UpdateSample(state, glide_polar, full_update);

  UpdateStatsSpeeds(state.time);
  UpdateFlightMode();

  assert(!force_full_update);

  return sample_updated || full_update;
}

void
AbstractTask::UpdateStatsSpeeds(const fixed time)
{
  if (!stats.task_finished) {
    stats_computer.total.CalcSpeeds(stats.total, time);
    stats_computer.current_leg.CalcSpeeds(stats.current_leg, time);
  }

  stats_computer.ComputeWindow(time, stats);
}

void
AbstractTask::UpdateStatsGlide(const AircraftState &state,
                               const GlidePolar &glide_polar)
{
  stats.glide_required = AngleToGradient(CalcRequiredGlide(state,
                                                           glide_polar));
}

void
AbstractTask::UpdateStatsTimes(const fixed time)
{
  if (!stats.task_finished) {
    stats.current_leg.SetTimes(fixed(0), ScanLegStartTime(), time);

    const fixed until_start_s = GetType() == TaskType::ORDERED &&
      GetActiveTaskPointIndex() == 0
      /* flying towards the start point in an ordered task: pass
         current_leg.time_remaining, which is the estimated time to
         reach the start point */
      ? stats.current_leg.time_remaining_now
      /* already beyond the start point (or no start point) */
      : fixed(0);

    stats.total.SetTimes(until_start_s, ScanTotalStartTime(), time);
  }
}

void
AbstractTask::ResetAutoMC()
{
  mc_lpf_valid = false;
}

void 
AbstractTask::Reset()
{
  ResetAutoMC();
  ce_lpf.Reset(fixed(1));
  stats.reset();
  force_full_update = true;
}

fixed
AbstractTask::CalcLegGradient(const AircraftState &aircraft) const
{
  // Get next turnpoint
  const TaskWaypoint *tp = GetActiveTaskPoint();
  if (!tp)
    return fixed(0);

  // Get the distance to the next turnpoint
  const fixed d = tp->GetVectorRemaining(aircraft.location).distance;
  if (!positive(d))
    return fixed(0);

  // Calculate the geometric gradient (height divided by distance)
  return (aircraft.altitude - tp->GetElevation()) / d;
}

bool 
AbstractTask::CalcEffectiveMC(const AircraftState &state_now,
                              const GlidePolar &glide_polar,
                              fixed &val) const
{
  val = glide_polar.GetMC();
  return true;
}

void
AbstractTask::UpdateFlightMode()
{
  stats.calc_flight_mode(task_behaviour);
}
