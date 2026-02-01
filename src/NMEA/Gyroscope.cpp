// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Gyroscope.hpp"

void
GyroscopeState::Complement(const GyroscopeState &add)
{
  if (add.available && (!available || (add.real && !real))) {
    real              = add.real;
    angular_rate_X    = add.angular_rate_X;
    angular_rate_Y    = add.angular_rate_Y;
    angular_rate_Z    = add.angular_rate_Z;
    available         = add.available;
    fixed_and_aligned = add.fixed_and_aligned;
  }
}
