/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "ExternalClock.hpp"
#include "BrokenTime.hpp"

fixed
ExternalClock::Apply(fixed fix_time, BrokenTime &broken_time)
{
  fixed hours, mins, secs;

  // Calculate Hour
  hours = fix_time / 10000;
  broken_time.hour = (int)hours;

  // Calculate Minute
  mins = fix_time / 100;
  mins = mins - fixed(broken_time.hour) * 100;
  broken_time.minute = (int)mins;

  // Calculate Second
  secs = fix_time - fixed(broken_time.hour * 10000 + broken_time.minute * 100);
  broken_time.second = (int)secs;

  // FixTime is now seconds-based instead of mixed format
  fix_time = secs + fixed(broken_time.minute * 60 + broken_time.hour * 3600);

  return fix_time;
}
