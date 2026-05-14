// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoBounds.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Math/Angle.hpp"

/**
 * Smallest #GeoBounds containing #center and the four cardinal points at
 * #radius_m (constant-bearing offsets).  Useful for framing a circular
 * neighbourhood with an axis-aligned geographic rectangle.
 */
[[gnu::pure]]
inline GeoBounds
SquareEnvelopeGeoBounds(const GeoPoint &center, double radius_m) noexcept
{
  GeoBounds b(center);
  for (int i = 0; i < 4; ++i)
    b.Extend(GeoVector(radius_m, Angle::Degrees(90. * i)).EndPoint(center));
  return b;
}
