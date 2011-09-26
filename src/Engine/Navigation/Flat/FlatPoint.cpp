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
#include "FlatPoint.hpp"
#include <algorithm>
#include <math.h>

fixed
FlatPoint::CrossProduct(const FlatPoint &p2) const
{
  return x * p2.y - p2.x * y;
}

void
FlatPoint::MultiplyY(const fixed a)
{
  y *= a;
}

void
FlatPoint::Subtract(const FlatPoint &p2)
{
  x -= p2.x;
  y -= p2.y;
}

void
FlatPoint::Add(const FlatPoint &p2)
{
  x += p2.x;
  y += p2.y;
}

void
FlatPoint::Rotate(const Angle angle)
{
  const fixed _x = x;
  const fixed _y = y;
  fixed sa, ca;
  angle.sin_cos(sa, ca);
  x = _x * ca - _y * sa;
  y = _x * sa + _y * ca;
}

fixed
FlatPoint::Distance(const FlatPoint &p) const
{
  return hypot(p.x - x, p.y - y);
}

fixed
FlatPoint::MagnitudeSquared() const {
  return sqr(x)+sqr(y);
}

fixed
FlatPoint::Magnitude() const
{
  return hypot(x, y);
}
