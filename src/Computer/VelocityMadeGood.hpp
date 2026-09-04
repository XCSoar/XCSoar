// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/SpeedVector.hpp"
#include "Math/Angle.hpp"

/**
 * Calculate the velocity made good (VMG) towards a target: the
 * component of a velocity along the bearing to that target.  This is
 * the rate at which the distance to the target shrinks while the
 * aircraft holds its current velocity.
 *
 * @param velocity the velocity over ground
 * @param target_bearing the bearing from the aircraft to the target
 * @return the velocity made good [m/s]; the full ground speed when
 * heading straight at the target, zero when flying at right angles to
 * it and negative when flying away from it
 */
[[gnu::const]]
static inline double
CalculateVelocityMadeGood(SpeedVector velocity,
                          Angle target_bearing) noexcept
{
  return velocity.norm * (target_bearing - velocity.bearing).cos();
}
