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
#include "TaskSolveTravelled.hpp"
#include <math.h>
#include "Util/Tolerances.hpp"

TaskSolveTravelled::TaskSolveTravelled(const std::vector<OrderedTaskPoint*>& tps,
                                       const unsigned activeTaskPoint,
                                       const AircraftState &_aircraft,
                                       const GlidePolar &gp,
                                       const fixed _xmin, 
                                       const fixed _xmax):
  ZeroFinder(_xmin, _xmax, fixed(TOLERANCE_CRUISE_EFFICIENCY)),
  aircraft(_aircraft),
  tm(tps, activeTaskPoint, gp)
{
  dt = aircraft.time-tps[0]->get_state_entered().time;
  if (positive(dt)) {
    inv_dt = fixed_one/dt;
  } else {
    inv_dt = fixed_zero; // error!
  }
}

#define SOLVE_ZERO

fixed 
TaskSolveTravelled::time_error() 
{
  res = tm.glide_solution(aircraft);
#ifdef SOLVE_ZERO
  fixed d = res.time_elapsed-dt;
#else
  fixed d = fabs(res.time_elapsed-dt);
#endif
  if (res.validity!=GlideResult::RESULT_OK) {
    d += res.time_virtual;
  }
  return d*inv_dt;
}

fixed 
TaskSolveTravelled::search(const fixed ce) 
{
#ifdef SOLVE_ZERO
  return find_zero(ce);
#else
  return find_min(ce);
#endif
}
