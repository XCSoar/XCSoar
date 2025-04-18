// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GeoVector.hpp"
#include "GeoPoint.hpp"
#include "Math.hpp"

GeoVector::GeoVector(const GeoPoint &source, const GeoPoint &target)
{
  *this = source.DistanceBearing(target);
}

GeoPoint
GeoVector::EndPoint(const GeoPoint &source) const
{
  if (distance <= 0)
    return source;

  return ::FindLatitudeLongitude(source, bearing, distance);
}

GeoPoint
GeoVector::MidPoint(const GeoPoint &source) const
{
  if (distance <= 0)
    return source;

  return ::FindLatitudeLongitude(source, bearing, distance / 2);
}
