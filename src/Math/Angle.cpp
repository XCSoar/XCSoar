/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifdef RADIANS
#define fixed_circle fixed_two_pi
#define fixed_half_circle fixed_pi
#else
#define fixed_circle fixed_360
#define fixed_half_circle fixed_180
#endif

int
Angle::sign() const
{
  if (positive(m_value)) return 1;
  if (negative(m_value)) return -1;
  return 0;
}

void
Angle::flip()
{
  m_value = -m_value;
}

fixed
Angle::magnitude_degrees() const 
{
  return fabs(value_degrees());
}

fixed
Angle::magnitude_radians() const 
{
  return fabs(value_radians());
}

Angle
Angle::flipped() const
{
  Angle retval(m_value);
  retval.flip();
  return retval;
}

Angle
Angle::as_bearing() const
{
  Angle retval(m_value);

  while (retval.m_value < fixed_zero)
    retval.m_value += fixed_circle;

  while (retval.m_value >= fixed_circle)
    retval.m_value -= fixed_circle;

  return retval;
}

Angle
Angle::as_delta() const
{
  Angle retval(m_value);
  while (retval.m_value <= -fixed_half_circle)
    retval.m_value += fixed_circle;

  while (retval.m_value > fixed_half_circle)
    retval.m_value -= fixed_circle;

  return retval;
}

Angle
Angle::Reciprocal() const
{
  Angle retval(m_value+ fixed_half_circle);
  return retval.as_bearing();
}

Angle
Angle::BiSector(const Angle &OutBound) const
{
  return Reciprocal().HalfAngle(OutBound);
}

Angle
Angle::HalfAngle(const Angle &End) const
{
  if (m_value == End.m_value) {
    return Reciprocal();
  } else if (m_value > End.m_value) {
    if ((m_value - End.m_value) < fixed_half_circle)
      return Angle((m_value + End.m_value) * fixed_half).Reciprocal();
    else
      return Angle((m_value + End.m_value) * fixed_half);
  } else {
    if ((End.m_value - m_value) < fixed_half_circle)
      return Angle((m_value + End.m_value) * fixed_half).Reciprocal();
    else
      return Angle((m_value + End.m_value) * fixed_half);
  }
}

Angle
Angle::Fraction(const Angle &End, const fixed fraction) const
{
  if (m_value == End.m_value)
    return Angle(m_value);

  const Angle diff = Angle(End.m_value - m_value).as_delta();

  // Note: do not return with as_bearing() since this will produce incorrect
  // results for latitude values!

  return Angle(m_value + diff.m_value * fraction);
}

gcc_pure
bool
Angle::between(const Angle &start, const Angle &end) const
{
  Angle width = (end - start).as_bearing();
  Angle delta = (*this - start).as_bearing();

  return delta <= width;
}
