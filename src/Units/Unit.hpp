// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class Unit: uint8_t {
  UNDEFINED,
  KILOMETER,
  NAUTICAL_MILES,
  STATUTE_MILES,
  KILOMETER_PER_HOUR,
  KNOTS,
  STATUTE_MILES_PER_HOUR,
  METER_PER_SECOND,
  FEET_PER_MINUTE,
  METER,
  FEET,
  FLIGHT_LEVEL,
  KELVIN,
  DEGREES_CELCIUS, // K = C° + 273,15
  DEGREES_FAHRENHEIT, // K = (°F + 459,67) / 1,8
  HECTOPASCAL,
  MILLIBAR,
  TORR,
  INCH_MERCURY,
  KG_PER_M2,
  LB_PER_FT2,
  KG,
  LB,
  PERCENT,
  GRADIENT,
  VOLT,
  HZ,
  RPM,
  /**
   * The sentinel: the number of units in this enum.
   */
  COUNT
};
