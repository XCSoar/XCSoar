// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <utility>

#include <math.h>

[[gnu::const]]
static inline std::pair<double, double>
sin_cos(const double thetha) noexcept {
////   aug: not used: #if __cplusplus >= 201703
////   aug: not used: sin_cos(const double thetha) noexcept {
////   aug: not used: #else
////   aug: not used: sin_cos(const double thetha) {
////   aug: not used: #endif
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
