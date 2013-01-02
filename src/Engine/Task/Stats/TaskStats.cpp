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

#include "TaskStats.hpp"
#include "Task/TaskBehaviour.hpp"

void
TaskStats::reset()
{
  total.Reset();
  current_leg.Reset();

  Time = fixed(0);
  glide_required = fixed(0);
  cruise_efficiency = fixed(1);
  effective_mc = fixed(0);
  mc_best = fixed(0);
  distance_nominal = fixed(0);
  distance_max = fixed(0);
  distance_min = fixed(0);
  distance_scored = fixed(0);
  task_valid = false;
  task_started = false;
  task_finished = false;
  flight_mode_final_glide = false;
  flight_mode_height_margin = 120;
}

bool
TaskStats::calc_flight_mode(const TaskBehaviour &settings)
{
  const int margin =
      (flight_mode_final_glide ? 1 : 0) * flight_mode_height_margin;

  /* when final glide Auto MacCready is enabled, it will auto-set MC
     to 0 if needed; therefore, we must use the "MC=0" solution to
     decide whether to switch to final glide */
  const GlideResult &solution_remaining = settings.IsAutoMCFinalGlideEnabled()
    ? total.solution_mc0
    : total.solution_remaining;

  const bool this_is_final = solution_remaining.IsOk() &&
    positive(solution_remaining.altitude_difference + fixed(margin));

  if (flight_mode_final_glide == this_is_final)
    return false;

  flight_mode_final_glide = this_is_final;
  return true;
}

void
TaskStatsComputer::reset(TaskStats &data)
{
  total.Reset(data.total);
  current_leg.Reset(data.current_leg);
}
