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

#include "BrokenTime.hpp"

#include <assert.h>

BrokenTime
BrokenTime::FromSecondOfDay(unsigned second_of_day)
{
  assert(second_of_day < 3600u * 24u);

  unsigned hour = second_of_day / 3600u;
  unsigned second_of_hour = second_of_day % 3600u;
  return BrokenTime(hour, second_of_hour / 60u, second_of_hour % 60u);
}

BrokenTime
BrokenTime::FromSecondOfDayChecked(unsigned second_of_day)
{
  return FromSecondOfDay(second_of_day % (3600u * 24u));
}

BrokenTime
BrokenTime::FromMinuteOfDay(unsigned minute_of_day)
{
  assert(minute_of_day < 60u * 24u);

  return BrokenTime(minute_of_day / 60u, minute_of_day % 60u);
}

BrokenTime
BrokenTime::FromMinuteOfDayChecked(unsigned minute_of_day)
{
  return FromMinuteOfDay(minute_of_day % (60u * 24u));
}

BrokenTime
BrokenTime::operator+(unsigned seconds) const
{
  assert(IsPlausible());

  seconds += GetSecondOfDay();
  return FromSecondOfDayChecked(seconds);
}

BrokenTime
BrokenTime::operator+(int seconds) const
{
  assert(IsPlausible());

  seconds += GetSecondOfDay();
  while (seconds < 0)
    seconds += 3600 * 24;

  return FromSecondOfDayChecked(seconds);
}
