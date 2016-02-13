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

#include "TaskGlideRequired.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Util/Tolerances.hpp"

TaskGlideRequired::TaskGlideRequired(const std::vector<OrderedTaskPoint *> &tps,
                                     const unsigned activeTaskPoint,
                                     const AircraftState &_aircraft,
                                     const GlideSettings &settings,
                                     const GlidePolar &_gp)
  :ZeroFinder(-10, 10, TOLERANCE_GLIDE_REQUIRED),
   tm(tps.cbegin(), tps.cend(), activeTaskPoint, settings, _gp),
   aircraft(_aircraft)
{
  // Vopt at mc=0
  tm.set_mc(0);
}

TaskGlideRequired::TaskGlideRequired(TaskPoint* tp,
                                     const AircraftState &_aircraft,
                                     const GlideSettings &settings,
                                     const GlidePolar &_gp)
  :ZeroFinder(-10, 10, TOLERANCE_GLIDE_REQUIRED),
   tm(tp, settings, _gp), // Vopt at mc=0
   aircraft(_aircraft)
{
  tm.set_mc(0);
}

double
TaskGlideRequired::f(const double S)
{
  res = tm.glide_sink(aircraft, S);
  return res.altitude_difference;
}

double
TaskGlideRequired::search(const double S)
{
  auto a = find_zero(S);
  return a/res.v_opt;
}
