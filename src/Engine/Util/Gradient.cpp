// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Gradient.hpp"

#include <algorithm> // for std::clamp()

#include <math.h>

double
AngleToGradient(const double d) noexcept
{
  if (d != 0) {
    return std::clamp(1. / d, -999., 999.);
  } else {
    return 999;
  }
}

bool
GradientValid(const double d) noexcept
{
  return fabs(d) < 999;
}
