/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

bool
FlatLine::IntersectOriginCircle(const double r,
                                FlatPoint &i1, FlatPoint &i2) const
{
  const auto d = GetVector();
  const auto dr = GetSquaredDistance();
  const auto D = CrossProduct();

  auto det = Square(r) * dr - Square(D);
  if (det < 0)
    // no solution
    return false;

  det = sqrt(det);
  const auto inv_dr = 1. / dr;
  i1.x = (D * d.y + copysign(d.x, d.y) * det) * inv_dr;
  i2.x = (D * d.y - copysign(d.x, d.y) * det) * inv_dr;
  i1.y = (-D * d.x + fabs(d.y) * det) * inv_dr;
  i2.y = (-D * d.x - fabs(d.y) * det) * inv_dr;
  return true;
}

bool
FlatLine::IntersectCircle(const double r, const FlatPoint c,
                          FlatPoint &i1, FlatPoint &i2) const
{
  const FlatLine that = *this - c;
  if (that.IntersectOriginCircle(r, i1, i2)) {
    i1 = i1 + c;
    i2 = i2 + c;
    return true;
  }

  return false;
}
