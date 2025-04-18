// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirDensity.hpp"

#include <math.h>

static constexpr double isa_sea_level_density = 1.225;
static constexpr double k4 = 44330.8;
static constexpr double k6 = 1.0 / 42266.5;
static constexpr double k7 = 1.0 / 0.234969;

double
AirDensity(const double altitude) noexcept
{
  return pow((k4 - altitude) * k6, k7);
}

double
AirDensityRatio(const double altitude) noexcept
{
  return sqrt(isa_sea_level_density / AirDensity(altitude));
}
