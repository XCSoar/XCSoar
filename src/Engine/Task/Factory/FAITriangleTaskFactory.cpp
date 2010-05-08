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
#include "FAITriangleTaskFactory.hpp"

FAITriangleTaskFactory::FAITriangleTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb):
  FAITaskFactory(_task, tb)
{
}

bool 
FAITriangleTaskFactory::validate()
{
  if (!FAITaskFactory::validate()) {
    return false;
  }

  if (m_task.task_size()==4) {

    const double d1 = m_task.getTaskPoint(1)->get_vector_planned().Distance / 1000;
    const double d2 = m_task.getTaskPoint(2)->get_vector_planned().Distance / 1000;
    const double d3 = m_task.getTaskPoint(3)->get_vector_planned().Distance / 1000;
    const double d_wp = d1+d2+d3;

    /**
     * A triangle is a valid FAI-triangle, if no side is less than
     * 28% of the total length (total length less than 750 km), or no
     * side is less than 25% or larger than 45% of the total length
     * (totallength >= 750km).
     */
 
    if( ( d_wp < 750.0 ) &&
        ( d1 >= 0.28 * d_wp && d2 >= 0.28 * d_wp && d3 >= 0.28 * d_wp ) )
      // small FAI
      return true;
    else if( d_wp >= 750.0 &&
             ( d1 > 0.25 * d_wp && d2 > 0.25 * d_wp && d3 > 0.25 * d_wp ) &&
             ( d1 <= 0.45 * d_wp && d2 <= 0.45 * d_wp && d3 <= 0.45 * d_wp ) )
      // large FAI
      return true;

    // distances out of limits
    return false;
  }

  // unknown task...
  return true;
}

void 
FAITriangleTaskFactory::update_ordered_task_behaviour(OrderedTaskBehaviour& to)
{
  FAITaskFactory::update_ordered_task_behaviour(to);
  to.min_points = 4;
  to.max_points = 4;
}
