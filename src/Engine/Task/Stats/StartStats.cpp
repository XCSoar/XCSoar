// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StartStats.hpp"
#include "Navigation/Aircraft.hpp"

void
StartStats::SetStarted(const AircraftState &aircraft)
{
  task_started = true;
  time = aircraft.time;
  altitude = aircraft.altitude;
  ground_speed = aircraft.ground_speed;
}
