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
#include "TaskMacCreadyRemaining.hpp"
#include "TaskSolution.hpp"

TaskMacCreadyRemaining::TaskMacCreadyRemaining(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _activeTaskPoint,
                                               const GlidePolar _gp):
  TaskMacCready(_tps,_activeTaskPoint, _gp)
{
  m_start = m_activeTaskPoint;
}

TaskMacCreadyRemaining::TaskMacCreadyRemaining(TaskPoint* tp,
                                               const GlidePolar _gp):
  TaskMacCready(tp,_gp)
{
}

GlideResult 
TaskMacCreadyRemaining::tp_solution(const unsigned i,
                                    const AIRCRAFT_STATE &aircraft, 
                                    fixed minH) const
{
  return TaskSolution::glide_solution_remaining(*m_tps[i],aircraft, m_glide_polar, minH);
}


const AIRCRAFT_STATE 
TaskMacCreadyRemaining::get_aircraft_start(const AIRCRAFT_STATE &aircraft) const
{
  return aircraft;
}

bool
TaskMacCreadyRemaining::has_targets() const
{
  for (int i=m_start; i<=m_end; i++) {
    if (m_tps[i]->has_target()) {
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
  for (int i=m_start; i<=m_end; i++) {
    modified |= m_tps[i]->set_range(tp,false);
  }
  if (!force_current && !modified) {
    // couldn't modify remaining targets, so force move even if inside
    for (int i=m_start; i<=m_end; i++) {
      if (m_tps[i]->set_range(tp,true)) {
        // quick exit
        return;
      }
    }
  }
}


void 
TaskMacCreadyRemaining::target_save()
{
  for (int i=m_start; i<=m_end; i++) {
      m_tps[i]->target_save();
  }
}

void 
TaskMacCreadyRemaining::target_restore()
{
  for (int i=m_start; i<=m_end; i++) {
      m_tps[i]->target_restore();
  }
}
