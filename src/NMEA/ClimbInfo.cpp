// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ClimbInfo.hpp"

void
OneClimbInfo::Clear()
{
  duration = {};
  gain = 0;
  lift_rate = 0;
  start_altitude = 0;
}

void
ClimbInfo::Clear()
{
  current_thermal.Clear();
  last_thermal.Clear();
}
