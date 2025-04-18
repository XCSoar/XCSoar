// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlyingState.hpp"

void
FlyingState::Reset()
{
  flying = false;
  on_ground = false;
  powered = false;
  flight_time = {};
  takeoff_time = TimeStamp::Undefined();
  takeoff_location.SetInvalid();
  release_time = TimeStamp::Undefined();
  release_location.SetInvalid();
  power_on_time = TimeStamp::Undefined();
  power_on_location.SetInvalid();
  power_off_time = TimeStamp::Undefined();
  power_off_location.SetInvalid();
  far_location.SetInvalid();
  far_distance = -1;
  landing_time = TimeStamp::Undefined();
  landing_location.SetInvalid();
}
