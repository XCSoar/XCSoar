/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

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
