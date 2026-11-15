// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

/**
 * What the tapes of the full-screen horizon show.  The defaults are
 * those of an attitude indicator: indicated airspeed, heading and the
 * height above the terrain below.  Pilots who want a navigation value
 * there choose it themselves.
 */
struct HorizonSettings {
  /** The speed tape on the left. */
  enum class Speed : uint8_t {
    INDICATED,
    TRUE_AIRSPEED,
    GROUND,
  } speed;

  /** The course tape along the top. */
  enum class Course : uint8_t {
    HEADING,
    TRACK,
  } course;

  /** The altitude tape on the right. */
  enum class Altitude : uint8_t {
    ABOVE_GROUND,
    BARO,
    GPS,
  } altitude;

  constexpr void SetDefaults() noexcept {
    speed = Speed::INDICATED;
    course = Course::HEADING;
    altitude = Altitude::ABOVE_GROUND;
  }
};
