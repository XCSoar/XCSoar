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

TaskMacCreadyRemaining::TaskMacCreadyRemaining(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _active_index,
                                               const GlideSettings &settings,
                                               const GlidePolar &_gp):
  TaskMacCready(_tps, _active_index, settings, _gp)
{
  start_index = active_index;
}

TaskMacCreadyRemaining::TaskMacCreadyRemaining(TaskPoint* tp,
                                               const GlideSettings &settings,
                                               const GlidePolar &_gp):
  TaskMacCready(tp, settings, _gp)
{
}

GlideResult 
TaskMacCreadyRemaining::tp_solution(const unsigned i,
                                    const AircraftState &aircraft, 
                                    fixed minH) const
{
  return TaskSolution::GlideSolutionRemaining(*points[i],aircraft,
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
  for (int i = start_index; i <= end_index; i++) {
    if (points[i]->HasTarget() && !points[i]->IsTargetLocked()) {
      return true;
    }
  }
  return false;
}


void 
TaskMacCreadyRemaining::set_range(const fixed tp, const bool force_current)
{
  // first try to modify targets without regard to current inside (unless forced)
  bool modified = force_current;
  for (int i = start_index; i <= end_index; i++) {
    modified |= points[i]->SetRange(tp,false);
  }
  if (!force_current && !modified) {
    // couldn't modify remaining targets, so force move even if inside
    for (int i = start_index; i <= end_index; i++) {
      if (points[i]->SetRange(tp,true)) {
        // quick exit
        return;
      }
    }
  }
}


void 
TaskMacCreadyRemaining::target_save()
{
  for (int i = start_index; i <= end_index; i++) {
      points[i]->SaveTarget();
  }
}

void 
TaskMacCreadyRemaining::target_restore()
{
  for (int i = start_index; i <= end_index; i++) {
      points[i]->RestoreTarget();
  }
}
