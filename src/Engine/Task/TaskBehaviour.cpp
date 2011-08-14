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

#include "TaskBehaviour.hpp"

void
SectorDefaults::SetDefaults()
{
  start_type = AbstractTaskFactory::START_CYLINDER;
  start_radius = fixed(1000);
  turnpoint_type = AbstractTaskFactory::AST_CYLINDER;
  turnpoint_radius = fixed(500);
  finish_type = AbstractTaskFactory::FINISH_CYLINDER;
  finish_radius = fixed(1000);
}

void
TaskStartMargins::SetDefaults()
{
  start_max_speed_margin = fixed_zero;
  start_max_height_margin = 0u;
}

void
TaskBehaviour::SetDefaults()
{
  TaskStartMargins::SetDefaults();

  optimise_targets_range = true;
  optimise_targets_bearing = true;
  optimise_targets_margin = 300;
  auto_mc = false;
  auto_mc_mode = AUTOMC_CLIMBAVERAGE;
  calc_cruise_efficiency = true;
  calc_effective_mc = true;
  calc_glide_required = true;
  goto_nonlandable = true;
  risk_gamma = fixed_zero;
  enable_olc = true;
  contest = OLC_Plus;
  contest_handicap = 100;
  safety_mc = fixed_half;
  safety_height_arrival = fixed(300);
  task_type_default = FACTORY_RT;
  sector_defaults.SetDefaults();
  ordered_defaults.SetDefaults();
  route_planner.SetDefaults();
  enable_trace = true;
  abort_task_mode = atmSimple;
}

void
TaskBehaviour::all_off()
{
  optimise_targets_range = false;
  optimise_targets_bearing = false;
  auto_mc = false;
  calc_cruise_efficiency = false;
  calc_glide_required = false;
  enable_olc = false;
  ordered_defaults.all_off();
  route_planner.mode = RoutePlannerConfig::rpNone;
}
