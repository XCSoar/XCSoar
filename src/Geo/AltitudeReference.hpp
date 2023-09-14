// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * This enum specifies the reference for altitude specifications.
 */
enum class AltitudeReference : uint8_t {
  /**
   * Altitude is measured above ground level (AGL).
   *
   * Note: the integer value is important because it is stored in the
   * profile.
   */
  AGL = 0,

  /**
   * Altitude is measured above mean sea level (MSL).
   *
   * Note: the integer value is important because it is stored in the
   * profile.
   */
  MSL = 1,

  /**
   * Altitude is measured above the standard pressure (1013.25 hPa).
   * This is used for flight levels (FL).
   */
  STD,
};
