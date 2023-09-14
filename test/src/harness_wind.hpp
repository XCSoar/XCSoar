// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"

#include <stdio.h>

static constexpr unsigned NUM_WIND = 9;

static inline double
wind_to_mag(int n_wind)
{
  if (n_wind)
    return ((n_wind - 1) / 4 + 1) * 5;

  return 0;
}

static inline Angle
wind_to_dir(int n_wind)
{
  if (n_wind)
    return Angle::Degrees(90 * ((n_wind - 1) % 4)).AsBearing();

  return Angle::Zero();
}

static inline const char*
wind_name(int n_wind)
{
  static char buffer[80];
  sprintf(buffer,"%d m/s @ %d", (int)wind_to_mag(n_wind),
          (int)wind_to_dir(n_wind).Degrees());
  return buffer;
}
