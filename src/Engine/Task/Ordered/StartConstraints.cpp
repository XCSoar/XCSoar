// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StartConstraints.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Navigation/Aircraft.hpp"

void
StartConstraints::SetDefaults()
{
  open_time_span = RoughTimeSpan::Invalid();
  max_speed = 0;
  max_height = 0;
  max_height_ref = AltitudeReference::AGL;
  require_arm = false;
  score_exit = true;
  fai_finish = false;
  pev_start_wait_time = {};
  pev_start_window = {};
}

bool
StartConstraints::CheckSpeed(double ground_speed,
                             const TaskStartMargins *margins) const
{
  if (max_speed == 0)
    return true;

  if (fai_finish)
    return true;

  const auto margin = margins != nullptr
    ? margins->max_speed_margin
    : 0;

  return ground_speed <= max_speed + margin;
}

bool
StartConstraints::CheckHeight(const AircraftState &state,
                              const double start_elevation,
                              const TaskStartMargins *margins) const
{
  if (max_height == 0)
    return true;

  if (fai_finish)
    return true;

  const unsigned margin = margins != nullptr
    ? margins->max_height_margin
    : 0u;

  if (max_height_ref == AltitudeReference::MSL)
    return state.altitude <= max_height + margin;
  else
    return state.altitude <= max_height + margin + start_elevation;
}
