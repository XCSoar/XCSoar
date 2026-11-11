// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "GlideSolvers/GlidePolar.hpp"

/**
 * A glider for tests and tools that need one to fly, because
 * #GlidePolar starts without a polar and without masses.
 */
inline GlidePolar
FlyablePolar(double mc) noexcept
{
  GlidePolar polar(mc);
  polar.SetCoefficients(PolarCoefficients(0.00157, -0.0734, 1.48), false);
  polar.SetReferenceMass(300, false);
  polar.SetEmptyMass(300, false);
  polar.SetCrewMass(90, false);
  polar.SetMaxBallast(90, false);
  polar.SetVMax(75, false);
  polar.Update();
  polar.SetMC(mc);
  return polar;
}
