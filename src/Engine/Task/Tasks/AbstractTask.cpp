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

AbstractTask::AbstractTask(enum type _type, TaskEvents &te,
                           const TaskBehaviour &tb, const GlidePolar &gp)
  :TaskInterface(_type),
   activeTaskPoint(0),
   activeTaskPoint_last(0-1),
   stats_computer(stats),
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
AbstractTask::update_auto_mc(GlidePolar &glide_polar,
                             const AircraftState& state, fixed fallback_mc)
{
  if (!positive(fallback_mc))
    fallback_mc = glide_polar.GetMC();

  if (!task_started(true) || !task_behaviour.auto_mc) {
    reset_auto_mc();
    return false;
  }

  if (task_behaviour.auto_mc_mode == TaskBehaviour::AUTOMC_CLIMBAVERAGE) {
    stats.mc_best = mc_lpf.reset(fallback_mc);
    trigger_auto = false;
    return false;
  }

  fixed mc_found;
  if (calc_mc_best(state, mc_found)) {
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
AbstractTask::update_idle(const AircraftState &state)
{
  if (task_started() && task_behaviour.calc_cruise_efficiency) {
    fixed val = fixed_one;
    if (calc_cruise_efficiency(state, val))
      stats.cruise_efficiency = ce_lpf.update(val);
  } else {
    stats.cruise_efficiency = ce_lpf.reset(fixed_one);
  }

  if (task_started() && task_behaviour.calc_effective_mc) {
    fixed val = glide_polar.GetMC();
    if (calc_effective_mc(state, val))
      stats.effective_mc = em_lpf.update(val);
  } else {
    stats.effective_mc = em_lpf.reset(glide_polar.GetMC());
  }

  if (task_behaviour.calc_glide_required)
    update_stats_glide(state);
  else
    stats.glide_required = fixed_zero; // error

  return false;
}

unsigned 
AbstractTask::getActiveTaskPointIndex() const
{
  return activeTaskPoint;
}

void
AbstractTask::update_stats_distances(const GeoPoint &location,
                                     const bool full_update)
{
  stats.total.remaining.set_distance(scan_distance_remaining(location));

  if (full_update)
    stats.distance_nominal = scan_distance_nominal();

  scan_distance_minmax(location, full_update,
                       &stats.distance_min, &stats.distance_max);

  stats.total.travelled.set_distance(scan_distance_travelled(location));
  stats.total.planned.set_distance(scan_distance_planned());

  if (is_scored()) {
    if (!task_started()) 
      stats.distance_scored = fixed_zero;
    else if (!task_finished()) 
      stats.distance_scored = scan_distance_scored(location);
  } else
    stats.distance_scored = fixed_zero;
}

void
AbstractTask::update_glide_solutions(const AircraftState &state)
{
  glide_solution_remaining(state, glide_polar, stats.total.solution_remaining,
                           stats.current_leg.solution_remaining);

  if (positive(glide_polar.GetMC())) {
    GlidePolar polar_mc0 = glide_polar;
    polar_mc0.SetMC(fixed_zero); 
    
    glide_solution_remaining(state, polar_mc0, stats.total.solution_mc0,
                             stats.current_leg.solution_mc0);
  } else {
    // no need to re-calculate, just copy
    stats.total.solution_mc0 = stats.total.solution_remaining;
    stats.current_leg.solution_mc0 = stats.current_leg.solution_remaining;
  }

  glide_solution_travelled(state, stats.total.solution_travelled,
                           stats.current_leg.solution_travelled);

  glide_solution_planned(state, stats.total.solution_planned,
                         stats.current_leg.solution_planned,
                         stats.total.remaining_effective,
                         stats.current_leg.remaining_effective,
                         stats.total.solution_remaining.time_elapsed,
                         stats.current_leg.solution_remaining.time_elapsed);

  stats.total.pirker.set_distance(
      stats.total.planned.get_distance() -
      stats.total.remaining_effective.get_distance());

  stats.current_leg.pirker.set_distance(
      stats.current_leg.planned.get_distance() -
      stats.current_leg.remaining_effective.get_distance());

  if (stats.current_leg.solution_remaining.IsDefined())
    stats.current_leg.remaining.set_distance(
        stats.current_leg.solution_remaining.vector.Distance);

  if (stats.current_leg.solution_travelled.IsDefined())
    stats.current_leg.travelled.set_distance(
        stats.current_leg.solution_travelled.vector.Distance);

  if (stats.current_leg.solution_planned.IsDefined())
    stats.current_leg.planned.set_distance(
        stats.current_leg.solution_planned.vector.Distance);

  stats.total.gradient = ::AngleToGradient(calc_gradient(state));
  stats.current_leg.gradient = ::AngleToGradient(leg_gradient(state));
}

bool
AbstractTask::update(const AircraftState &state, 
                     const AircraftState &state_last)
{
  stats.task_valid = check_task();
  stats.has_targets = has_targets();

  const bool full_update = 
    check_transitions(state, state_last) ||
    (activeTaskPoint != activeTaskPoint_last);

  update_stats_times(state);
  update_stats_distances(state.location, full_update);
  update_glide_solutions(state);
  bool sample_updated = update_sample(state, full_update);
  update_stats_speeds(state, state_last);
  update_flight_mode();

  activeTaskPoint_last = activeTaskPoint;

  return sample_updated || full_update;
}

void
AbstractTask::update_stats_speeds(const AircraftState &state, 
                                  const AircraftState &state_last)
{
  if (!task_finished()) {
    if (task_started()) {
      const fixed dt = state.time - state_last.time;
      stats_computer.total.calc_speeds(dt);
      stats_computer.current_leg.calc_speeds(dt);
    } else {
      stats_computer.total.reset();
      stats_computer.current_leg.reset();
    }
  }
}

void
AbstractTask::update_stats_glide(const AircraftState &state)
{
  stats.glide_required = AngleToGradient(calc_glide_required(state));
}

void
AbstractTask::update_stats_times(const AircraftState &state)
{
  // default for tasks with no start time...
  stats.Time = state.time;
  if (!task_finished()) {
    stats.total.set_times(scan_total_start_time(state), state);
    stats.current_leg.set_times(scan_leg_start_time(state),state);
  }
}

void
AbstractTask::reset_auto_mc()
{
  stats.mc_best = mc_lpf.reset(glide_polar.GetMC());
  trigger_auto = false;
}

void 
AbstractTask::reset()
{
  reset_auto_mc();
  activeTaskPoint_last = 0 - 1;
  ce_lpf.reset(fixed_one);
  stats_computer.reset();
}

fixed
AbstractTask::leg_gradient(const AircraftState &aircraft) const
{
  // Get next turnpoint
  const TaskWaypoint *tp = getActiveTaskPoint();
  if (!tp)
    return fixed_zero;

  // Get the distance to the next turnpoint
  const fixed d = tp->get_vector_remaining(aircraft).Distance;
  if (!d)
    return fixed_zero;

  // Calculate the geometric gradient (height divided by distance)
  return (aircraft.altitude - tp->get_elevation()) / d;
}

bool 
AbstractTask::calc_effective_mc(const AircraftState &state_now, fixed& val) const
{
  val = glide_polar.GetMC();
  return true;
}

void
AbstractTask::update_flight_mode()
{
  if (!stats.calc_flight_mode())
    return;

  task_events.transition_flight_mode(stats.flight_mode_final_glide);
}
