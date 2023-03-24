// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*! @file
 * @brief Library for calculating Earth dimensions on the FAI sphere
 */

#pragma once

#include "Math/Angle.hpp"

namespace FAISphere {
  static constexpr unsigned REARTH = 6371000;

  /**
   * Convert a distance on earth's surface [m] to the according Angle,
   * assuming the earth is a sphere.
   */
  constexpr
  static inline Angle
  EarthDistanceToAngle(double distance)
  {
    return Angle::Radians(distance / REARTH);
  }

  /**
   * Convert an angle to the according distance on earth's surface [m],
   * assuming the earth is a sphere.
   */
  constexpr
  static inline double
  AngleToEarthDistance(Angle angle)
  {
    return angle.Radians() * REARTH;
  }
}
