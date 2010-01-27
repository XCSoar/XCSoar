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

#include "Math/Geometry.hpp"
#include "Math/FastMath.h"

#include <math.h>

/**
 * Limits the angle (theta) to 0 - 360 degrees
 * @param theta Input angle
 * @return Output angle (0-360 degrees)
 */
fixed
AngleLimit360(const fixed &theta)
{
  fixed retval = theta;

  while (retval < fixed_zero)
    retval += fixed_360;

  while (retval > fixed_360)
    retval -= fixed_360;

  return retval;
}

/**
 * Limits the angle (theta) to -180 - +180 degrees
 * @param theta Input angle
 * @return Output angle (-180 - +180 degrees)
 */
fixed
AngleLimit180(const fixed &theta)
{
  fixed retval = theta;
  while (retval < -fixed_180)
    retval += fixed_360;

  while (retval > fixed_180)
    retval -= fixed_360;

  return retval;
}

/**
 * Rotate angle by 180 degrees and limit to 0 - 360 degrees
 * @param InBound Input angle
 * @return Output angle (0 - 360 degrees)
 */
fixed
Reciprocal(const fixed &InBound)
{
  return AngleLimit360(InBound + fixed_180);
}

fixed
BiSector(const fixed &InBound, const fixed &OutBound)
{
  return HalfAngle(Reciprocal(InBound), OutBound);
}

fixed
HalfAngle(const fixed &Start, const fixed &End)
{
  if (Start == End) {
    return Reciprocal(Start);
  } else if (Start > End) {
    if ((Start - End) < fixed_180)
      return Reciprocal((Start + End) * fixed_half);
    else
      return (Start + End) * fixed_half;
  } else {
    if ((End - Start) < fixed_180)
      return Reciprocal((Start + End) * fixed_half);
    else
      return (Start + End) * fixed_half;
  }
}

