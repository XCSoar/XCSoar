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
#include "TaskBestMc.hpp"
#include <math.h>
#include "Util/Tolerances.hpp"
#include <algorithm>

/// @todo only engage this class if above final glide at mc=0

TaskBestMc::TaskBestMc(const std::vector<OrderedTaskPoint*>& tps,
                       const unsigned activeTaskPoint,
                       const AircraftState &_aircraft,
                       const GlidePolar &_gp,
                       const fixed _mc_min):
  ZeroFinder(_mc_min, fixed(10.0), fixed(TOLERANCE_BEST_MC)),
  tm(tps,activeTaskPoint,_gp),
  aircraft(_aircraft) 
{
}

TaskBestMc::TaskBestMc(TaskPoint* tp,
                       const AircraftState &_aircraft,
                       const GlidePolar &_gp):
  ZeroFinder(fixed(0.1), fixed(10.0), fixed(TOLERANCE_BEST_MC)),
  tm(tp,_gp),
  aircraft(_aircraft) 
{
}

#define fixed_tiny fixed(0.001)

fixed 
TaskBestMc::f(const fixed mc) 
{
  tm.set_mc(max(fixed_tiny, mc));
  res = tm.glide_solution(aircraft);

  return res.altitude_difference;
}

bool TaskBestMc::valid(const fixed mc) 
{
  return res.IsOk()
    && (res.altitude_difference >= -tolerance*fixed_two*res.vector.distance);
}

fixed 
TaskBestMc::search(const fixed mc) 
{
  // only search if mc zero is valid
  f(fixed_zero);
  if (valid(fixed_zero)) {
    fixed a = find_zero(mc);
    if (valid(a)) {
      return a;
    }
  }
  return mc;
}

bool 
TaskBestMc::search(const fixed mc, fixed& result) 
{
  // only search if mc zero is valid
  f(fixed_zero);
  if (valid(fixed_zero)) {
    fixed a = find_zero(mc);
    if (valid(a)) {
      result = a;
      return true;
    }
  }
  result = mc;
  return false;
}

