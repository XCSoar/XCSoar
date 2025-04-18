// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace WGS84 {
  static constexpr double EQUATOR_RADIUS = 6378137;
  static constexpr double FLATTENING = 1 / 298.257223563;
  static constexpr double POLE_RADIUS = EQUATOR_RADIUS * (1 - FLATTENING);
}
