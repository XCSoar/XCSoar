// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

namespace LiveTrack24 {

enum class VehicleType {
  PARAGLIDER = 1,
  FLEX_WING_FAI1 = 2,
  RIGID_WING_FAI5 = 4,
  GLIDER = 8,
  PARAMOTOR = 16,
  TRIKE = 32,
  POWERED_AIRCRAFT = 64,
  HOT_AIR_BALLOON = 128,

  WALK = 16385,
  RUN = 16386,
  BIKE = 16388,

  HIKE = 16400,
  CYCLE = 16401,
  MOUNTAIN_BIKE = 16402,
  MOTORCYCLE = 16403,

  WINDSURF = 16500,
  KITESURF = 16501,
  SAILING = 16502,

  SNOWBOARD = 16600,
  SKI = 16601,
  SNOWKITE = 16602,

  CAR = 17100,
  CAR_4X4 = 17101,
};

} // namespace Livetrack24
