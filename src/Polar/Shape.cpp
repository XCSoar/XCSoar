// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Polar/Polar.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"

PolarCoefficients
PolarShape::CalculateCoefficients() const noexcept
{
  return PolarCoefficients::From3VW(points[0].v, points[1].v, points[2].v,
                                    points[0].w, points[1].w, points[2].w);
}

bool
PolarShape::IsValid() const noexcept
{
  return CalculateCoefficients().IsValid();
}
