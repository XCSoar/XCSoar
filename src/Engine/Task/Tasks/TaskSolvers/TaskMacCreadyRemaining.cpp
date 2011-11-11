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
#include "TaskMacCreadyRemaining.hpp"
#include "TaskSolution.hpp"

TaskMacCreadyRemaining::TaskMacCreadyRemaining(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _activeTaskPoint,
                                               const GlidePolar &_gp):
  TaskMacCready(_tps,_activeTaskPoint, _gp)
{
  m_start = m_activeTaskPoint;
}

TaskMacCreadyRemaining::TaskMacCreadyRemaining(TaskPoint* tp,
                                               const GlidePolar &_gp):
  TaskMacCready(tp,_gp)
{
}

GlideResult 
TaskMacCreadyRemaining::tp_solution(const unsigned i,
                                    const AircraftState &aircraft, 
                                    fixed minH) const
{
  return TaskSolution::glide_solution_remaining(*m_tps[i],aircraft, m_glide_polar, minH);
}


const AircraftState &
TaskMacCreadyRemaining::get_aircraft_start(const AircraftState &aircraft) const
{
  return aircraft;
}

bool
TaskMacCreadyRemaining::has_targets() const
{
  for (int i = m_start; i <= m_end; i++) {
    if (m_tps[i]->HasTarget() && !m_tps[i]->IsTargetLocked()) {
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
  for (int i = m_start; i <= m_end; i++) {
    modified |= m_tps[i]->SetRange(tp,false);
  }
  if (!force_current && !modified) {
    // couldn't modify remaining targets, so force move even if inside
    for (int i = m_start; i <= m_end; i++) {
      if (m_tps[i]->SetRange(tp,true)) {
        // quick exit
        return;
      }
    }
  }
}


void 
TaskMacCreadyRemaining::target_save()
{
  for (int i = m_start; i <= m_end; i++) {
      m_tps[i]->SaveTarget();
  }
}

void 
TaskMacCreadyRemaining::target_restore()
{
  for (int i = m_start; i <= m_end; i++) {
      m_tps[i]->RestoreTarget();
  }
}
