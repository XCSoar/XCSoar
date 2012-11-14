/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Angle.hpp"
#include <assert.h>

void
Angle::ToDMS(unsigned &dd, unsigned &mm, unsigned &ss, bool &is_positive) const
{
  is_positive = !negative(value);

  unsigned value = uround(AbsoluteDegrees() * 3600);

  ss = value % 60;
  value /= 60;

  mm = value % 60;
  value /= 60;

  dd = value;
}

int
Angle::Sign(const fixed& tolerance) const
{
  if ((value > tolerance))
    return 1;
  if ((value < -tolerance))
    return -1;

  return 0;
}

int
Angle::Sign() const
{
  if (positive(value))
    return 1;
  if (negative(value))
    return -1;

  return 0;
}

void
Angle::Flip()
{
  value = -value;
}

fixed
Angle::AbsoluteDegrees() const 
{
  return Absolute().Degrees();
}

fixed
Angle::AbsoluteRadians() const 
{
  return Absolute().Radians();
}

Angle
Angle::Flipped() const
{
  Angle retval(value);
  retval.Flip();
  return retval;
}

Angle
Angle::AsBearing() const
{
  assert(fabs(value) < fixed(100) * FullCircle().Native());

  Angle retval(value);

  while (retval < Zero())
    retval += FullCircle();

  while (retval >= FullCircle())
    retval -= FullCircle();

  return retval;
}

Angle
Angle::AsDelta() const
{
  assert(fabs(value) < fixed(100) * FullCircle().Native());

  Angle retval(value);

  while (retval <= -HalfCircle())
    retval += FullCircle();

  while (retval > HalfCircle())
    retval -= FullCircle();

  return retval;
}

Angle
Angle::Reciprocal() const
{
  return (*this + HalfCircle()).AsBearing();
}

Angle
Angle::HalfAngle(const Angle end) const
{
  if (value == end.value) {
    return Reciprocal();
  } else if (value > end.value) {
    if ((*this - end) < HalfCircle())
      return (*this + end).Half().Reciprocal();
    else
      return (*this + end).Half();
  } else {
    if ((end - *this) < HalfCircle())
      return (*this + end).Half().Reciprocal();
    else
      return (*this + end).Half();
  }
}

Angle
Angle::Fraction(const Angle end, const fixed fraction) const
{
  if (value == end.value)
    return Angle(value);

  const Angle diff = Angle(end.value - value).AsDelta();

  // Note: do not return with AsBearing() since this will produce incorrect
  // results for latitude values!

  return Angle(value + diff.value * fraction);
}

gcc_pure
bool
Angle::Between(const Angle start, const Angle end) const
{
  Angle width = (end - start).AsBearing();
  Angle delta = (*this - start).AsBearing();

  return delta <= width;
}
