// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskBehaviour.hpp"

void
SectorDefaults::SetDefaults()
{
  start_type = TaskPointFactoryType::START_CYLINDER;
  start_radius = 1000;
  turnpoint_type = TaskPointFactoryType::AST_CYLINDER;
  turnpoint_radius = 500;
  finish_type = TaskPointFactoryType::FINISH_CYLINDER;
  finish_radius = 1000;
}

void
TaskStartMargins::SetDefaults()
{
  max_speed_margin = 0;
  max_height_margin = 0u;
}

void
TaskBehaviour::SetDefaults()
{
  glide.SetDefaults();

  optimise_targets_range = true;
  optimise_targets_bearing = true;
  optimise_targets_margin = std::chrono::minutes{5};
  auto_mc = false;
  auto_mc_mode = AutoMCMode::CLIMBAVERAGE;
  calc_cruise_efficiency = true;
  calc_effective_mc = true;
  calc_glide_required = true;
  goto_nonlandable = true;
  risk_gamma = 0;
  safety_mc = 0.5;
  safety_height_arrival = 300;
  task_type_default = TaskFactoryType::RACING;
  start_margins.SetDefaults();
  sector_defaults.SetDefaults();
  ordered_defaults.SetDefaults();
  route_planner.SetDefaults();
  abort_task_mode = AbortTaskMode::SIMPLE;
}

void
TaskBehaviour::DisableAll()
{
  optimise_targets_range = false;
  optimise_targets_bearing = false;
  auto_mc = false;
  calc_cruise_efficiency = false;
  calc_glide_required = false;
  route_planner.mode = RoutePlannerConfig::Mode::NONE;
}
