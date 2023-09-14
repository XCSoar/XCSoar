// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Polar/Polar.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"

PolarCoefficients
PolarInfo::CalculateCoefficients() const noexcept
{
  return shape.CalculateCoefficients();
}
