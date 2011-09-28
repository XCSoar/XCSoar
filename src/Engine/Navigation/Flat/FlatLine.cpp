/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#define sgn(x,y) (negative(x) ? -y : y)

FlatPoint
FlatLine::ave() const
{
  return (p1 + p2).Half();
}

fixed
FlatLine::dx() const
{
  return p2.x - p1.x;
}

fixed
FlatLine::dy() const
{
  return p2.y - p1.y;
}

fixed
FlatLine::cross() const
{
  return p1.CrossProduct(p2);
}

void
FlatLine::mul_y(const fixed a)
{
  p1.MultiplyY(a);
  p2.MultiplyY(a);
}

fixed
FlatLine::d() const
{
  return sqrt(dsq());
}

fixed
FlatLine::dsq() const
{
  return sqr(dx()) + sqr(dy());
}

void
FlatLine::sub(const FlatPoint&p)
{
  p1.Subtract(p);
  p2.Subtract(p);
}

void
FlatLine::add(const FlatPoint&p)
{
  p1.Add(p);
  p2.Add(p);
}

Angle
FlatLine::angle() const
{
  return Angle::Radians(atan2(dy(), dx()));
}

void
FlatLine::rotate(const Angle theta)
{
  p1.Rotate(theta);
  p2.Rotate(theta);
}

bool
FlatLine::intersect_czero(const fixed r, FlatPoint &i1, FlatPoint &i2) const
{
  const fixed _dx = dx();
  const fixed _dy = dy();
  const fixed dr = dsq();
  const fixed D = cross();

  fixed det = sqr(r) * dr - sqr(D);
  if (negative(det))
    // no solution
    return false;

  det = sqrt(det);
  const fixed inv_dr = fixed_one / dr;
  i1.x = (D * _dy + sgn(_dy, _dx) * det) * inv_dr;
  i2.x = (D * _dy - sgn(_dy, _dx) * det) * inv_dr;
  i1.y = (-D * _dx + fabs(_dy) * det) * inv_dr;
  i2.y = (-D * _dx - fabs(_dy) * det) * inv_dr;
  return true;
}

bool
FlatLine::intersect_circle(const fixed r, const FlatPoint c,
                           FlatPoint &i1, FlatPoint &i2) const
{
  FlatLine that = *this;
  that.sub(c);
  if (that.intersect_czero(r, i1, i2)) {
    i1 = i1 + c;
    i2 = i2 + c;
    return true;
  }

  return false;
}

fixed
FlatLine::dot(const FlatLine& that) const
{
  return (p2 - p1).DotProduct(that.p2 - that.p1);
}
