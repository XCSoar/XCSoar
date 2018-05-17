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

#include "TaskSolveTravelled.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Util/Tolerances.hpp"

TaskSolveTravelled::TaskSolveTravelled(const std::vector<OrderedTaskPoint *> &tps,
                                       const unsigned activeTaskPoint,
                                       const AircraftState &_aircraft,
                                       const GlideSettings &settings,
                                       const GlidePolar &gp,
                                       const double _xmin,
                                       const double _xmax)
  :ZeroFinder(_xmin, _xmax, TOLERANCE_CRUISE_EFFICIENCY),
   aircraft(_aircraft),
   tm(tps.cbegin(), activeTaskPoint, settings, gp)
{
  dt = aircraft.time - tps[0]->GetEnteredState().time;
  if (dt > 0) {
    inv_dt = 1. / dt;
  } else {
    inv_dt = 0; // error!
  }
}

#define SOLVE_ZERO

double
TaskSolveTravelled::time_error()
{
  GlideResult res = tm.glide_solution(aircraft);
  if (!res.IsOk())
    /* what can we do if there's no solution?  This is an attempt to
       make ZeroFinder ignore this call, by returning a large value.
       I'm not sure if this kludge is correct. */
    return 999999;

#ifdef SOLVE_ZERO
  auto d = res.time_elapsed - dt;
#else
  auto d = fabs(res.time_elapsed - dt);
#endif
  d += res.time_virtual;

  return d * inv_dt;
}

double
TaskSolveTravelled::search(const double ce)
{
#ifdef SOLVE_ZERO
  return find_zero(ce);
#else
  return find_min(ce);
#endif
}
