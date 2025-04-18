// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <utility>

#include <math.h>

[[gnu::const]]
static inline std::pair<double, double>
sin_cos(const double thetha) noexcept
{
  double s, c;
#ifdef __APPLE__
  __sincos(thetha, &s, &c);
#elif defined(_MSC_VER)
  // STL and MSVC have no sincos...
  s = sin(thetha);
  c = cos(thetha);
#else
  sincos(thetha, &s, &c);
#endif
  return std::make_pair(s, c);
}
