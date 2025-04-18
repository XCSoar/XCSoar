// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlatGeoPoint.hpp"
#include "Math/FastMath.hpp"

unsigned
FlatGeoPoint::Distance(const FlatGeoPoint &sp) const noexcept
{
  const FlatGeoPoint delta = *this - sp;
  return ihypot(delta.x, delta.y);
}

unsigned
FlatGeoPoint::DistanceSquared(const FlatGeoPoint &sp) const noexcept
{
  const FlatGeoPoint delta = *this - sp;
  return delta.MagnitudeSquared();
}
