/*
Copyright_License {

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

#include "ARange.hpp"

bool
AngleRange::Extend(Angle value)
{
  const Angle length = GetLength();

  /* check if value is nearer to start or to end, and extend that
     side */

  Angle start_delta = (start - value).AsBearing();
  if (start_delta + length >= Angle::FullCircle())
    /* it's inside, no change required */
    return false;

  Angle end_delta = (value - end).AsBearing();
  if (start_delta < end_delta)
    start = value;
  else
    end = value;

  return true;
}

bool
AngleRange:: IntersectWith(const AngleRange &other) {
  bool result = false;

  if (IsInside(other.start)) {
    start = other.start;
    result = true;
  } else if (other.IsInside(start))
    result = true;

  if (IsInside(other.end)) {
    end = other.end;
    result = true;
  } else if (other.IsInside(end))
    result = true;

  return result;
}
