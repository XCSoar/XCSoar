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

AbstractTask::AbstractTask(enum type _type, TaskEvents &te,
                           const TaskBehaviour &tb, const GlidePolar &gp):
  TaskInterface(_type),
  activeTaskPoint(0),
  activeTaskPoint_last(0-1),
  stats_computer(stats),
  task_events(te),
  task_behaviour(tb),
  glide_polar(gp),
  mc_lpf(fixed(8)),
  ce_lpf(fixed(60)),
  em_lpf(fixed(60)),
  trigger_auto(false) {}

bool 
AbstractTask::update_auto_mc(GlidePolar &glide_polar,
                             const AIRCRAFT_STATE& state,
                             fixed fallback_mc)
{
  if (!positive(fallback_mc))
    fallback_mc = glide_polar.get_mc();

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
    glide_polar.set_mc(stats.mc_best);
  } else {
    // reset lpf so will be smooth next time it becomes active
    stats.mc_best = mc_lpf.reset(fallback_mc);
  }

  return trigger_auto;
}

bool 
AbstractTask::update_idle(const AIRCRAFT_STATE &state)
{
  if (task_started() && task_behaviour.calc_cruise_efficiency) {
    fixed val = fixed_one;
    if (calc_cruise_efficiency(state, val)) {
      stats.cruise_efficiency = ce_lpf.update(val);
    }
  } else {
    stats.cruise_efficiency = ce_lpf.reset(fixed_one);
  }

  if (task_started() && task_behaviour.calc_effective_mc) {
    fixed val = glide_polar.get_mc();
    if (calc_effective_mc(state, val)) {
      stats.effective_mc = em_lpf.update(val);
    }
  } else {
    stats.effective_mc = em_lpf.reset(glide_polar.get_mc());
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
AbstractTask::update_glide_solutions(const AIRCRAFT_STATE &state)
{
  glide_solution_remaining(state, glide_polar, stats.total.solution_remaining,
                           stats.current_leg.solution_remaining);

  if (positive(glide_polar.get_mc())) {
    GlidePolar polar_mc0 = glide_polar;
    polar_mc0.set_mc(fixed_zero); 
    
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
                         stats.total.solution_remaining.TimeElapsed,
                         stats.current_leg.solution_remaining.TimeElapsed);

  stats.total.pirker.set_distance(stats.total.planned.get_distance()
                                  -stats.total.remaining_effective.get_distance());
  stats.current_leg.pirker.set_distance(stats.current_leg.planned.get_distance()
                                  -stats.current_leg.remaining_effective.get_distance());

  stats.current_leg.remaining.set_distance(
    stats.current_leg.solution_remaining.Vector.Distance);
  stats.current_leg.travelled.set_distance(
    stats.current_leg.solution_travelled.Vector.Distance);
  stats.current_leg.planned.set_distance(
    stats.current_leg.solution_planned.Vector.Distance);

  stats.total.gradient = ::AngleToGradient(calc_gradient(state));
  stats.current_leg.gradient = ::AngleToGradient(leg_gradient(state));
}

bool
AbstractTask::update(const AIRCRAFT_STATE &state, 
                     const AIRCRAFT_STATE &state_last)
{
  stats.task_valid = check_task();
  stats.has_targets = has_targets();

  const bool full_update = 
    check_transitions(state, state_last) ||
    (activeTaskPoint != activeTaskPoint_last);

  update_stats_times(state);
  update_stats_distances(state.Location, full_update);
  update_glide_solutions(state);
  bool sample_updated = update_sample(state, full_update);
  update_stats_speeds(state, state_last);
  update_flight_mode();

  activeTaskPoint_last = activeTaskPoint;

  return sample_updated || full_update;
}

void
AbstractTask::update_stats_speeds(const AIRCRAFT_STATE &state, 
                                  const AIRCRAFT_STATE &state_last)
{
  const fixed dt = state.Time-state_last.Time;
  if (!task_finished()) {
    if (task_started()) {
      stats_computer.total.calc_speeds(dt);
      stats_computer.current_leg.calc_speeds(dt);
    } else {
      stats_computer.total.reset();
      stats_computer.current_leg.reset();
    }
  }
}

void
AbstractTask::update_stats_glide(const AIRCRAFT_STATE &state)
{
  stats.glide_required = AngleToGradient(calc_glide_required(state));
}

void
AbstractTask::update_stats_times(const AIRCRAFT_STATE &state)
{
  // default for tasks with no start time...
  stats.Time = state.Time;
  if (!task_finished()) {
    stats.total.set_times(scan_total_start_time(state), state);
    stats.current_leg.set_times(scan_leg_start_time(state),state);
  }
}

void
AbstractTask::reset_auto_mc()
{
  stats.mc_best = mc_lpf.reset(glide_polar.get_mc());
  trigger_auto = false;
}

void 
AbstractTask::reset()
{
  reset_auto_mc();
  activeTaskPoint_last = 0-1;
  ce_lpf.reset(fixed_one);
  stats_computer.reset();
}

fixed
AbstractTask::leg_gradient(const AIRCRAFT_STATE &aircraft) const
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
  return (aircraft.NavAltitude - tp->get_elevation()) / d;
}

bool 
AbstractTask::calc_effective_mc(const AIRCRAFT_STATE &state_now, fixed& val) const
{
  val = glide_polar.get_mc();
  return true;
}

void
AbstractTask::update_flight_mode()
{
  if (!stats.calc_flight_mode())
    return;

  task_events.transition_flight_mode(stats.flight_mode_final_glide);
}
