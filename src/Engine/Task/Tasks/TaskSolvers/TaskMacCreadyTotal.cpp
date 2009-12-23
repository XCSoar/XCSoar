/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
                                const AIRCRAFT_STATE &aircraft, 
                                fixed minH) const
{
  return TaskSolution::glide_solution_planned(*m_tps[i],aircraft, m_glide_polar, minH);
}

const AIRCRAFT_STATE 
TaskMacCreadyTotal::get_aircraft_start(const AIRCRAFT_STATE &aircraft) const
{
  if (m_tps[0]->has_entered()) {
    return m_tps[0]->get_state_entered();
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
    if (positive(m_gs[i].TimeElapsed)) {
      fixed p = (time_remaining-t_total)/m_gs[i].TimeElapsed;
      if ((p>=fixed_zero) && (p<=fixed_one)) {
        return d_total+p*m_gs[i].Vector.Distance;
      }
      d_total += m_gs[i].Vector.Distance;
      t_total += m_gs[i].TimeElapsed;
    }
  }
  return d_total;
}

fixed 
TaskMacCreadyTotal::effective_leg_distance(const fixed time_remaining) const
{
  fixed p = (time_remaining)/m_gs[m_activeTaskPoint].TimeElapsed;
  return p*m_gs[m_activeTaskPoint].Vector.Distance;
}

