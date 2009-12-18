/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "BaseTask/TaskPoint.hpp"
#include "Util/Gradient.hpp"


bool 
AbstractTask::update_idle(const AIRCRAFT_STATE &state)
{
  bool retval = false;
  if (task_started() && task_behaviour.auto_mc) {
    double mc_found = calc_mc_best(state);
    if (trigger_auto || (mc_found > stats.mc_best)) {
      trigger_auto = true;
      stats.mc_best = mc_lpf.update(mc_found);
      glide_polar.set_mc(stats.mc_best);
    } else {
      stats.mc_best = mc_lpf.reset(glide_polar.get_mc());
    }

    retval = true;
  } else {
    trigger_auto = false;
    stats.mc_best = mc_lpf.reset(glide_polar.get_mc());
  }
  if (task_started() && task_behaviour.calc_cruise_efficiency) {
    stats.cruise_efficiency = ce_lpf.update(calc_cruise_efficiency(state));
    retval = true;
  } else {
    stats.cruise_efficiency = ce_lpf.reset(1.0);
  }

  if (task_behaviour.calc_glide_required) {
    update_stats_glide(state);
  } else {
    stats.glide_required = fixed_zero; // error
  }

  return false;
}


unsigned 
AbstractTask::getActiveTaskPointIndex() const
{
  return activeTaskPoint;
}

void
AbstractTask::update_stats_distances(const GEOPOINT &location,
                                     const bool full_update)
{

  stats.total.remaining.set_distance(scan_distance_remaining(location));

  if (full_update) {
    stats.distance_nominal = scan_distance_nominal();
  }

  scan_distance_minmax(location, 
                       full_update,
                       &stats.distance_min,
                       &stats.distance_max);

  stats.total.travelled.set_distance(scan_distance_travelled(location));
  stats.total.planned.set_distance(scan_distance_planned());

  if (task_behaviour.task_scored) {
    stats.distance_scored = scan_distance_scored(location);
  } else {
    stats.distance_scored = 0;
  }
}

void
AbstractTask::update_glide_solutions(const AIRCRAFT_STATE &state)
{
  glide_solution_remaining(state, 
                           glide_polar,
                           stats.total.solution_remaining,
                           stats.current_leg.solution_remaining);

  if (positive(glide_polar.get_mc())) {
    GlidePolar polar_mc0 = glide_polar;
    polar_mc0.set_mc(fixed_zero);
    
    glide_solution_remaining(state, 
                             polar_mc0,
                             stats.total.solution_mc0,
                             stats.current_leg.solution_mc0);
  } else {
    // no need to re-calculate, just copy
    stats.total.solution_mc0 = stats.total.solution_remaining;
    stats.current_leg.solution_mc0 = stats.current_leg.solution_remaining;
  }

  glide_solution_travelled(state, 
                           stats.total.solution_travelled,
                           stats.current_leg.solution_travelled);

  glide_solution_planned(state, 
                         stats.total.solution_planned,
                         stats.current_leg.solution_planned,
                         stats.total.remaining_effective,
                         stats.current_leg.remaining_effective,
                         stats.total.solution_remaining.TimeElapsed,
                         stats.current_leg.solution_remaining.TimeElapsed);

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

  const bool full_update = 
    check_transitions(state, state_last) ||
    (activeTaskPoint != activeTaskPoint_last);

  update_stats_times(state);

  update_stats_distances(state.Location, full_update);

  update_glide_solutions(state);

  bool sample_updated = update_sample(state, full_update);

  update_stats_speeds(state, state_last);

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
      stats.total.calc_speeds(dt);
      stats.current_leg.calc_speeds(dt);
    } else {
      stats.total.reset();
      stats.current_leg.reset();
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
AbstractTask::reset()
{
  mc_lpf.reset(glide_polar.get_mc());
  trigger_auto = false;
  activeTaskPoint_last = 0-1;
  ce_lpf.reset(1.0);
  stats.reset();
}


double
AbstractTask::leg_gradient(const AIRCRAFT_STATE &aircraft) 
{
  TaskPoint *tp = getActiveTaskPoint();
  if (!tp) {
    return 0.0;
  }
  const fixed d = tp->get_vector_remaining(aircraft).Distance;
  if (d) {
    return (aircraft.Altitude-tp->get_elevation())/d;
  } else {
    return 0.0;
  }
}
