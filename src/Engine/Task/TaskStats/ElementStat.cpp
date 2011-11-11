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

#include "ElementStat.hpp"
#include "Navigation/Aircraft.hpp"
#include <algorithm>

void
ElementStat::Reset()
{
  time_started = fixed_minus_one;
  time_elapsed = time_remaining = time_planned = fixed_zero;
  gradient = fixed_zero;

  remaining_effective.Reset();
  remaining.Reset();
  planned.Reset();
  travelled.Reset();
  pirker.Reset();

  solution_planned.Reset();
  solution_travelled.Reset();
  solution_remaining.Reset();
  solution_mc0.Reset();
}

void
ElementStat::SetTimes(const fixed ts, const AircraftState& state)
{
  time_started = ts;
  if (negative(time_started))
    /* not yet started */
    time_elapsed = fixed_zero;
  else
    time_elapsed = max(state.time - fixed(ts), fixed_zero);

  if (solution_remaining.IsOk()) {
    time_remaining = solution_remaining.time_elapsed;
    time_planned = time_elapsed + time_remaining;
  } else {
    time_remaining = time_planned = fixed_zero;
  }
}

void
ElementStatComputer::Reset()
{
  initialised = false;

  CalcSpeeds(fixed_zero);
}

void 
ElementStatComputer::CalcSpeeds(const fixed dt)
{
  remaining_effective.calc_speed(data.time_remaining);
  remaining.calc_speed(data.time_remaining);
  planned.calc_speed(data.time_planned);
  travelled.calc_speed(data.time_elapsed);
  pirker.calc_speed(data.time_elapsed);

  if (!initialised) {
    if (positive(dt) && data.time_elapsed > fixed(15))
      initialised = true;

    data.vario.reset(data.solution_remaining);
    remaining_effective.calc_incremental_speed(fixed_zero);
    remaining.calc_incremental_speed(fixed_zero);
    planned.calc_incremental_speed(fixed_zero);
    travelled.calc_incremental_speed(fixed_zero);
    pirker.calc_incremental_speed(fixed_zero);
    return;
  }

  if (!positive(dt))
    return;

  remaining.calc_incremental_speed(dt);
  planned.calc_incremental_speed(dt);
  travelled.calc_incremental_speed(dt);

  if (data.solution_remaining.IsOk()) {
    remaining_effective.calc_incremental_speed(dt);
    pirker.calc_incremental_speed(dt);
    data.vario.update(data.solution_remaining, fixed(dt));
  } else {
    remaining_effective.calc_incremental_speed(fixed_zero);
    pirker.calc_incremental_speed(fixed_zero);
    data.vario.reset(data.solution_remaining);
  }
}

ElementStatComputer::ElementStatComputer(ElementStat &_data)
  :data(_data),
   remaining_effective(data.remaining_effective),
   remaining(data.remaining),
   planned(data.planned),
   travelled(data.travelled, false),
   pirker(data.pirker, false),
   initialised(false) {}
