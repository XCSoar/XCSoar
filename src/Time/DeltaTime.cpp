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

#include "DeltaTime.hpp"

#include <assert.h>

double
DeltaTime::Update(double current_time, double min_delta, double warp_tolerance)
{

  assert(current_time >= 0);
  assert(min_delta >= 0);
  assert(warp_tolerance >= 0);

  if (!IsDefined()) {
    /* first call */
    last_time = current_time;
    return 0;
  }

  if (current_time < last_time) {
    /* time warp */

    const auto delta = last_time - current_time;
    last_time = current_time;
    return delta < warp_tolerance ? 0 : -1;
  }

  const auto delta = current_time - last_time;
  if (delta < min_delta)
    /* difference too small, don't update "last" time stamp to let
       small differences add up eventually */
    return 0;

  last_time = current_time;

  if (delta > 4 * 3600)
    /* after several hours without a signal, we can assume there was
       a time warp */
    return -1;

  return delta;
}
