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

#include "TaskStats.hpp"
#include "Task/TaskBehaviour.hpp"

/** Margin for final glide flight mode transition (m) */
static constexpr int flight_mode_height_margin = 120;

void
TaskStats::reset()
{
  total.Reset();
  current_leg.Reset();

  glide_required = 0;
  cruise_efficiency = 1;
  effective_mc = 0;
  mc_best = 0;
  distance_nominal = 0;
  distance_max = 0;
  distance_min = 0;
  distance_scored = 0;

  inst_speed_slow = -1;
  inst_speed_fast = -1;

  active_index = 0;
  task_valid = false;
  has_targets = false;
  is_mat = false;
  has_optional_starts = false;
  task_finished = false;
  inside_oz = false;
  need_to_arm = false;
  flight_mode_final_glide = false;
  start.Reset();
  last_hour.Reset();
}

bool
TaskStats::calc_flight_mode(const TaskBehaviour &settings)
{
  const int margin = flight_mode_final_glide
    ? flight_mode_height_margin
    : 0;

  /* when final glide Auto MacCready is enabled, it will auto-set MC
     to 0 if needed; therefore, we must use the "MC=0" solution to
     decide whether to switch to final glide */
  const GlideResult &solution_remaining = settings.IsAutoMCFinalGlideEnabled()
    ? total.solution_mc0
    : total.solution_remaining;

  const bool this_is_final = solution_remaining.IsOk() &&
    solution_remaining.altitude_difference + margin > 0;

  if (flight_mode_final_glide == this_is_final)
    return false;

  flight_mode_final_glide = this_is_final;
  return true;
}
