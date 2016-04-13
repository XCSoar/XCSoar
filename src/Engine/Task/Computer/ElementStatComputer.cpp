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

#include "ElementStatComputer.hpp"
#include "Task/Stats/ElementStat.hpp"

ElementStatComputer::ElementStatComputer()
  :remaining_effective(),
   remaining(),
   planned(),
   travelled(false),
   vario(),
   initialised(false) {}

void
ElementStatComputer::Reset(ElementStat &data)
{
  initialised = false;

  CalcSpeeds(data, -1);
}

void 
ElementStatComputer::CalcSpeeds(ElementStat &data, const double time)
{
  remaining_effective.CalcSpeed(data.remaining_effective,
                                data.time_remaining_start);
  remaining.CalcSpeed(data.remaining, data.time_remaining_now);
  planned.CalcSpeed(data.planned, data.time_planned);
  travelled.CalcSpeed(data.travelled, data.time_elapsed);

  if (!initialised) {
    if (data.time_elapsed > 15)
      initialised = true;

    vario.reset(data.vario, data.solution_remaining);
    remaining_effective.ResetIncrementalSpeed(data.remaining_effective);
    remaining.ResetIncrementalSpeed(data.remaining);
    planned.ResetIncrementalSpeed(data.planned);
    travelled.ResetIncrementalSpeed(data.travelled);
    return;
  }

  remaining.CalcIncrementalSpeed(data.remaining, time);
  planned.CalcIncrementalSpeed(data.planned, time);
  travelled.CalcIncrementalSpeed(data.travelled, time);

  if (data.solution_remaining.IsOk()) {
    remaining_effective.CalcIncrementalSpeed(data.remaining_effective, time);
    vario.update(data.vario, data.solution_remaining);
  } else {
    remaining_effective.ResetIncrementalSpeed(data.remaining_effective);
    vario.reset(data.vario, data.solution_remaining);
  }
}
