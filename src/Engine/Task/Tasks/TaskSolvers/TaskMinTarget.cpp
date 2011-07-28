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
#include "TaskMinTarget.hpp"
#include <math.h>
#include "Util/Tolerances.hpp"


TaskMinTarget::TaskMinTarget(const std::vector<OrderedTaskPoint*>& tps,
                             const unsigned activeTaskPoint,
                             const AircraftState &_aircraft,
                             const GlidePolar &_gp,
                             const fixed _t_remaining,
                             StartPoint *_ts):
  ZeroFinder(fixed(0.0), fixed(1.0), fixed(TOLERANCE_MIN_TARGET)),
  tm(tps,activeTaskPoint,_gp),
  aircraft(_aircraft),
  t_remaining(_t_remaining),
  tp_start(_ts),
  force_current(false)
{

}

fixed 
TaskMinTarget::f(const fixed p) 
{
  // set task targets
  set_range(p);

  res = tm.glide_solution(aircraft);
  return res.time_elapsed - t_remaining;
}


bool 
TaskMinTarget::valid(const fixed tp) 
{
  //  const fixed ff = f(tp);
  return (res.validity == GlideResult::RESULT_OK); // && (ff>= -tolerance*fixed_two);
}

fixed 
TaskMinTarget::search(const fixed tp) 
{
  if (!tm.has_targets()) {
    // don't bother if nothing to adjust
    return tp;
  }

  force_current = false;
  /// @todo if search fails, force current
  const fixed p = find_zero(tp);
  if (valid(p)) {
    return p;
  } else {
    force_current = true;
    return find_zero(tp);
  }
}

void 
TaskMinTarget::set_range(const fixed p)
{
  tm.set_range(p, force_current);
  tp_start->scan_distance_remaining(aircraft.location);
}
