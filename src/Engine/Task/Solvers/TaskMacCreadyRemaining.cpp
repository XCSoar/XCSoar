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

#include "TaskMacCreadyRemaining.hpp"
#include "TaskSolution.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Task/Ordered/Points/AATPoint.hpp"

GlideResult
TaskMacCreadyRemaining::SolvePoint(const TaskPoint &tp,
                                   const AircraftState &aircraft,
                                   fixed minH) const
{
  return TaskSolution::GlideSolutionRemaining(tp, aircraft,
                                              settings, glide_polar, minH);
}


const AircraftState &
TaskMacCreadyRemaining::get_aircraft_start(const AircraftState &aircraft) const
{
  return aircraft;
}

bool
TaskMacCreadyRemaining::has_targets() const
{
  for (const TaskPoint *point : points)
    if (point->HasTarget() && !point->IsTargetLocked())
      return true;

  return false;
}


void 
TaskMacCreadyRemaining::set_range(const fixed tp, const bool force_current)
{
  // first try to modify targets without regard to current inside (unless forced)
  bool modified = force_current;
  for (TaskPoint *point : points)
    modified |= point->SetRange(tp,false);

  if (!force_current && !modified) {
    // couldn't modify remaining targets, so force move even if inside
    for (TaskPoint *point : points)
      if (point->SetRange(tp,true))
        // quick exit
        return;
  }
}


void 
TaskMacCreadyRemaining::target_save()
{
  auto saved = saved_targets.begin();
  for (TaskPoint *point : points)
    *saved++ = point->HasTarget()
      ? ((AATPoint *)point)->GetTarget()
      : GeoPoint::Invalid();
}

void 
TaskMacCreadyRemaining::target_restore()
{
  auto saved = saved_targets.cbegin();
  for (TaskPoint *point : points) {
    if (saved->IsValid()) {
      assert(point->HasTarget());
      ((AATPoint *)point)->SetTarget(*saved, true);
    }

    ++saved;
  }
}
