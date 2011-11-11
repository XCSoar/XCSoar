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
#include "TaskMacCreadyTotal.hpp"
#include "TaskSolution.hpp"

TaskMacCreadyTotal::TaskMacCreadyTotal(const std::vector<OrderedTaskPoint*> &_tps,
                                       const unsigned _activeTaskPoint,
                                       const GlidePolar &_gp):
  TaskMacCready(_tps,_activeTaskPoint, _gp)
{
}


GlideResult 
TaskMacCreadyTotal::tp_solution(const unsigned i,
                                const AircraftState &aircraft, 
                                fixed minH) const
{
  return TaskSolution::glide_solution_planned(*m_tps[i],aircraft, m_glide_polar, minH);
}

const AircraftState 
TaskMacCreadyTotal::get_aircraft_start(const AircraftState &aircraft) const
{
  if (m_tps[0]->HasEntered()) {
    return m_tps[0]->GetEnteredState();
  } else {
    return aircraft;
  }
}

fixed 
TaskMacCreadyTotal::effective_distance(const fixed time_remaining) const
{

  fixed t_total = fixed_zero;
  fixed d_total = fixed_zero;
  for (int i=m_end; i>=m_start; i--) {
    if (m_gs[i].IsOk() && positive(m_gs[i].time_elapsed)) {
      fixed p = (time_remaining-t_total)/m_gs[i].time_elapsed;
      if ((p>=fixed_zero) && (p<=fixed_one)) {
        return d_total+p*m_gs[i].vector.Distance;
      }
      d_total += m_gs[i].vector.Distance;
      t_total += m_gs[i].time_elapsed;
    }
  }
  return d_total;
}

fixed 
TaskMacCreadyTotal::effective_leg_distance(const fixed time_remaining) const
{
  fixed p = (time_remaining)/m_gs[m_activeTaskPoint].time_elapsed;
  return p*m_gs[m_activeTaskPoint].vector.Distance;
}

