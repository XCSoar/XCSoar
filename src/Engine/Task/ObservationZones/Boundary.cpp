// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Boundary.hpp"
#include "Geo/GeoVector.hpp"

#include <algorithm>

void
OZBoundary::GenerateArcExcluding(const GeoPoint &center, double radius,
                                 Angle start_radial, Angle end_radial) noexcept
{
  const unsigned steps = std::max(20,std::min(360, (int) (radius / 25)));
  const Angle delta = Angle::FullCircle() / steps;
  const Angle start = start_radial.AsBearing();
  Angle end = end_radial.AsBearing();
  if (end <= start + Angle::FullCircle() / 512)
    end += Angle::FullCircle();

  GeoVector vector(radius, start + delta);
  for (; vector.bearing < end; vector.bearing += delta)
    push_front(vector.EndPoint(center));
}
