// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CirclingInfo.hpp"

void
CirclingInfo::Clear()
{
  turning = false;
  circling = false;

  turn_rate = turn_rate_heading = Angle::Zero();

  time_cruise = {};
  time_circling = {};
  time_climb_noncircling = {};
  time_climb_circling = {};
  total_height_gain = 0;

  cruise_start_time = TimeStamp::Undefined();
  climb_start_time = TimeStamp::Undefined();

  max_height_gain = 0;

  turn_rate_smoothed = turn_rate_heading_smoothed = Angle::Zero();
  turn_mode = CirclingMode::CRUISE;

  circling_percentage = -1;
  noncircling_climb_percentage = -1;
  circling_climb_percentage = -1;
}
