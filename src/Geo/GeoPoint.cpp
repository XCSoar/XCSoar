// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GeoPoint.hpp"
#include "GeoVector.hpp"
#include "Math.hpp"
#include "SimplifiedMath.hpp"

GeoPoint
GeoPoint::Parametric(const GeoPoint &delta, const double t) const noexcept
{
  return (*this) + delta * t;
}

GeoPoint
GeoPoint::Interpolate(const GeoPoint &end, const double t) const noexcept
{
  return (*this) + (end - (*this)) * t;
}

double
GeoPoint::Distance(const GeoPoint &other) const noexcept
{
  return ::Distance(*this, other);
}

Angle
GeoPoint::Bearing(const GeoPoint &other) const noexcept
{
  return ::Bearing(*this, other);
}

GeoVector
GeoPoint::DistanceBearing(const GeoPoint &other) const noexcept
{
  GeoVector gv;
  ::DistanceBearing(*this, other, &gv.distance, &gv.bearing);
  return gv;
}

double
GeoPoint::DistanceS(const GeoPoint &other) const noexcept
{
  double distance;
  ::DistanceBearingS(*this, other, &distance, nullptr);
  return distance;
}

Angle
GeoPoint::BearingS(const GeoPoint &other) const noexcept
{
  Angle angle;
  ::DistanceBearingS(*this, other, (Angle *)nullptr, &angle);
  return angle;
}

GeoVector
GeoPoint::DistanceBearingS(const GeoPoint &other) const noexcept
{
  GeoVector gv;
  ::DistanceBearingS(*this, other, &gv.distance, &gv.bearing);
  return gv;
}

double
GeoPoint::ProjectedDistance(const GeoPoint &from,
                            const GeoPoint &to) const noexcept
{
  return ::ProjectedDistance(from, to, *this);
}

GeoPoint
GeoPoint::Middle(const GeoPoint &other) const noexcept
{
  return ::Middle(*this, other);
}

GeoPoint
GeoPoint::IntermediatePoint(const GeoPoint &destination,
                            const double distance) const noexcept
{
  return ::IntermediatePoint(*this, destination, distance);
}
