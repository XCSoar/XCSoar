/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Math/FastMath.h"

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
Angle::sin() const 
{
  return ::sin(m_value);
}

fixed
Angle::cos() const 
{
  return ::cos(m_value);
}

fixed
Angle::fastsine() const 
{
  return (::fastsine(value_degrees()));
}

fixed
Angle::fastcosine() const 
{
  return (::fastcosine(value_degrees()));
}

fixed
Angle::invfastcosine() const 
{
  return (::invfastcosine(value_degrees()));
}

int
Angle::ifastcosine() const 
{
  return (::ifastcosine(value_degrees()));
}

int
Angle::ifastsine() const 
{
  return (::ifastsine(value_degrees()));
}

void 
Angle::sin_cos(fixed& s, fixed& c) const
{
  return ::sin_cos(m_value, &s, &c);
}

fixed
Angle::magnitude() const 
{
  return fabs(m_value);
}

Angle
Angle::AngleLimit360() const
{
  Angle retval(m_value);

  while (retval.m_value < fixed_zero)
    retval.m_value += fixed_360;

  while (retval.m_value > fixed_360)
    retval.m_value -= fixed_360;

  return retval;
}

Angle
Angle::AngleLimit180() const
{
  Angle retval(m_value);
  while (retval.m_value < -fixed_180)
    retval.m_value += fixed_360;

  while (retval.m_value > fixed_180)
    retval.m_value -= fixed_360;

  return retval;
}

Angle
Angle::Reciprocal() const
{
  Angle retval(m_value+ fixed_180);
  return retval.AngleLimit360();
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
    if ((m_value - End.m_value) < fixed_180)
      return Angle((m_value + End.m_value) * fixed_half).Reciprocal();
    else
      return Angle((m_value + End.m_value) * fixed_half);
  } else {
    if ((End.m_value - m_value) < fixed_180)
      return Angle((m_value + End.m_value) * fixed_half).Reciprocal();
    else
      return Angle((m_value + End.m_value) * fixed_half);
  }
}


