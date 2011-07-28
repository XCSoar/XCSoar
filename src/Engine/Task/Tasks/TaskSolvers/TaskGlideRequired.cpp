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
#include "TaskGlideRequired.hpp"
#include <math.h>
#include "Util/Tolerances.hpp"

TaskGlideRequired::TaskGlideRequired(const std::vector<OrderedTaskPoint*>& tps,
                                     const unsigned activeTaskPoint,
                                     const AIRCRAFT_STATE &_aircraft,
                                     const GlidePolar &_gp):
  ZeroFinder(-fixed_ten, fixed_ten, fixed(TOLERANCE_GLIDE_REQUIRED)),
  tm(tps,activeTaskPoint,_gp), 
  aircraft(_aircraft) 
{
  // Vopt at mc=0
  tm.set_mc(fixed_zero);
}

TaskGlideRequired::TaskGlideRequired(TaskPoint* tp,
                                     const AIRCRAFT_STATE &_aircraft,
                                     const GlidePolar &_gp):
  ZeroFinder(-fixed_ten, fixed_ten, fixed(TOLERANCE_GLIDE_REQUIRED)),
  tm(tp,_gp), // Vopt at mc=0
  aircraft(_aircraft) 
{
  tm.set_mc(fixed_zero);
}

fixed 
TaskGlideRequired::f(const fixed S) 
{
  res = tm.glide_sink(aircraft, S);
  return res.altitude_difference;
}

fixed 
TaskGlideRequired::search(const fixed S) 
{
  fixed a = find_zero(S);
  return a/res.v_opt;
}
