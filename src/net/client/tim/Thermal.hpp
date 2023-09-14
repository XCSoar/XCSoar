// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"

#include <chrono>

namespace TIM {

struct Thermal {
  std::chrono::system_clock::time_point time;
  GeoPoint location;
  float climb_rate;
};

} // namespace TIM
