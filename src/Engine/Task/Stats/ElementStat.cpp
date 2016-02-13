/* Copyright_License {

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

#include "ElementStat.hpp"

void
ElementStat::Reset()
{
  location_remaining = GeoPoint::Invalid();
  vector_remaining = GeoVector::Invalid();
  next_leg_vector = GeoVector::Invalid();

  time_started = -1;
  time_elapsed = time_remaining_now = time_remaining_start = time_planned = 0;
  gradient = 0;

  remaining_effective.Reset();
  remaining.Reset();
  planned.Reset();
  travelled.Reset();
  pirker.Reset();

  solution_planned.Reset();
  solution_travelled.Reset();
  solution_remaining.Reset();
  solution_mc0.Reset();

  vario.Reset();
}

void
ElementStat::SetTimes(const double until_start_s, const double ts,
                      const double time)
{
  time_started = ts;

  if (time_started < 0 || time < 0)
    /* not yet started */
    time_elapsed = 0;
  else
    time_elapsed = fdim(time, ts);

  if (solution_remaining.IsOk()) {
    time_remaining_now = solution_remaining.time_elapsed;
    time_remaining_start = fdim(time_remaining_now, until_start_s);
    time_planned = time_elapsed + time_remaining_start;
  } else {
    time_remaining_now = time_remaining_start = time_planned = 0;
  }
}
