// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlatRay.hpp"
#include "Math/FastMath.hpp"

#include <stdlib.h>

#define sgn(x) (x >= 0 ? 1 : -1)

int
FlatRay::Magnitude() const noexcept
{
  return ihypot(vector.x, vector.y);
}

std::pair<int, int>
FlatRay::IntersectsRatio(const FlatRay &that) const noexcept
{
  std::pair<int, int> r;
  r.second = vector.CrossProduct(that.vector);
  if (r.second == 0)
    // lines are parallel
    return r;

  const FlatGeoPoint delta = that.point - point;
  r.first = delta.CrossProduct(that.vector);
  if (sgn(r.first) * sgn(r.second) < 0 || abs(r.first) > abs(r.second)) {
    // outside first line
    r.second = 0;
    return r;
  }

  const int ub = delta.CrossProduct(vector);
  if (sgn(ub) * sgn(r.second) < 0 || abs(ub) > abs(r.second)) {
    // outside second line
    r.second = 0;
    return r;
  }

  // inside both lines
  return r;
}

FlatGeoPoint
FlatRay::Parametric(const double t) const noexcept
{
  FlatGeoPoint p = point;
  p.x += iround(vector.x * t);
  p.y += iround(vector.y * t);
  return p;
}

double
FlatRay::Intersects(const FlatRay &that) const noexcept
{
  std::pair<int, int> r = IntersectsRatio(that);
  if (r.second == 0)
    return -1;
  return double(r.first) / double(r.second);
}

bool
FlatRay::IntersectsDistinct(const FlatRay &that) const noexcept
{
  std::pair<int, int> r = IntersectsRatio(that);
  return r.second != 0 &&
    sgn(r.second) * r.first > 0 &&
    abs(r.first) < abs(r.second);
}

double
FlatRay::DistinctIntersection(const FlatRay &that) const noexcept
{
  std::pair<int, int> r = IntersectsRatio(that);
  if (r.second != 0 &&
      sgn(r.second) * r.first > 0 &&
      abs(r.first) < abs(r.second)) {
    return double(r.first) / double(r.second);
  }

  return -1;
}
