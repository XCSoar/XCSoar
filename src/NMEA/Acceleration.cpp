// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Acceleration.hpp"

void
AccelerationState::Complement(const AccelerationState &add)
{
  if (add.available && (!available || (add.real && !real))) {
    real = add.real;
    g_load = add.g_load;
    available = add.available;
  }
}
