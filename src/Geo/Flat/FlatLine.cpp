// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlatLine.hpp"
#include "Math/Util.hpp"
#include "Math/Angle.hpp"

Angle
FlatLine::GetAngle() const
{
  const auto v = GetVector();
  return Angle::FromXY(v.x, v.y);
}

void
FlatLine::Rotate(const Angle theta)
{
  a.Rotate(theta);
  b.Rotate(theta);
}

std::optional<std::pair<FlatPoint, FlatPoint>>
FlatLine::IntersectOriginCircle(const double r) const noexcept
{
  // http://mathworld.wolfram.com/Circle-LineIntersection.html
  const auto d = GetVector();
  const auto dr = GetSquaredDistance();
  const auto D = CrossProduct();

  auto det = Square(r) * dr - Square(D);
  if (det < 0)
    // no solution
    return std::nullopt;

  det = sqrt(det);
  const auto inv_dr = 1. / dr;
  const auto sign_dx = (d.y < 0) ? -d.x : d.x;
  return std::pair<FlatPoint, FlatPoint>{
    {
      (D * d.y + sign_dx * det) * inv_dr,
      (-D * d.x + fabs(d.y) * det) * inv_dr,
    },
    {
      (D * d.y - sign_dx * det) * inv_dr,
      (-D * d.x - fabs(d.y) * det) * inv_dr,
    },
  };
}

std::optional<std::pair<FlatPoint, FlatPoint>>
FlatLine::IntersectCircle(const double r, const FlatPoint c) const noexcept
{
  const FlatLine that = *this - c;

  auto result = that.IntersectOriginCircle(r);
  if (result) {
    result->first += c;
    result->second += c;
  }

  return result;
}
