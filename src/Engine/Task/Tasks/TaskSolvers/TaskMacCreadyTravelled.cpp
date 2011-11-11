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
#include "TaskMacCreadyTravelled.hpp"
#include "TaskSolution.hpp"

TaskMacCreadyTravelled::TaskMacCreadyTravelled(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _activeTaskPoint,
                                               const GlidePolar &_gp):
  TaskMacCready(_tps,_activeTaskPoint, _gp)
{
  m_end = m_activeTaskPoint;
}


GlideResult 
TaskMacCreadyTravelled::tp_solution(const unsigned i,
                                    const AircraftState &aircraft, 
                                    fixed minH) const
{
  return TaskSolution::glide_solution_travelled(*m_tps[i],aircraft, m_glide_polar, minH);
}

const AircraftState &
TaskMacCreadyTravelled::get_aircraft_start(const AircraftState &aircraft) const
{
  if (m_tps[0]->HasEntered()) {
    return m_tps[0]->GetEnteredState();
  } else {
    return aircraft;
  }
}


fixed 
TaskMacCreadyTravelled::get_min_height(const AircraftState &aircraft) const 
{
  return aircraft.altitude;
}
