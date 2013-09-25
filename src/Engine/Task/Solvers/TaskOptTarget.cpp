/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "TaskOptTarget.hpp"
#include "Task/Ordered/Points/AATPoint.hpp"
#include "Task/Ordered/Points/StartPoint.hpp"
#include "Util/Tolerances.hpp"
#include "Util/Clamp.hpp"

TaskOptTarget::TaskOptTarget(const std::vector<OrderedTaskPoint*>& tps,
                             const unsigned activeTaskPoint,
                             const AircraftState &_aircraft,
                             const GlideSettings &settings,
                             const GlidePolar &_gp,
                             AATPoint &_tp_current,
                             const TaskProjection &projection,
                             StartPoint *_ts)
  :ZeroFinder(fixed(0.02), fixed(0.98), fixed(TOLERANCE_OPT_TARGET)),
   tm(tps.cbegin(), tps.cend(), activeTaskPoint, settings, _gp,
      /* ignore the travel to the start point */
      false),
   aircraft(_aircraft),
   tp_start(_ts),
   tp_current(_tp_current),
   iso(_tp_current, projection)
{
}

fixed
TaskOptTarget::f(const fixed p)
{
  // set task targets
  SetTarget(p);

  res = tm.glide_solution(aircraft);

  return res.time_elapsed;
}

bool
TaskOptTarget::valid(const fixed tp)
{
  f(tp);
  return res.IsOk();
}

fixed
TaskOptTarget::search(const fixed tp)
{
  if (tp_current.IsTargetLocked()) {
    // can't move, don't bother
    return fixed(-1);
  }
  if (iso.IsValid()) {
    tm.target_save();
    const fixed t = find_min(tp);
    if (!valid(t)) {
      // invalid, so restore old value
      tm.target_restore();
      return fixed(-1);
    } else {
      return t;
    }
  } else {
    return fixed(-1);
  }
}

void
TaskOptTarget::SetTarget(const fixed p)
{
  const GeoPoint loc = iso.Parametric(Clamp(p, xmin, xmax));
  tp_current.SetTarget(loc);
  tp_start->ScanDistanceRemaining(aircraft.location);
}
