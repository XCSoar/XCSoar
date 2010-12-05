/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "FAITriangleTaskFactory.hpp"
#include "Task/Tasks/OrderedTask.hpp"

FAITriangleTaskFactory::FAITriangleTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb):
  FAITaskFactory(_task, tb)
{
}

bool 
FAITriangleTaskFactory::validate()
{

  bool valid = FAITaskFactory::validate();

  if (m_task.task_size()==4) {
    const fixed d1 = m_task.getTaskPoint(1)->get_vector_planned().Distance;
    const fixed d2 = m_task.getTaskPoint(2)->get_vector_planned().Distance;
    const fixed d3 = m_task.getTaskPoint(3)->get_vector_planned().Distance;
    const fixed d_wp = d1+d2+d3;

    /**
     * A triangle is a valid FAI-triangle, if no side is less than
     * 28% of the total length (total length less than 750 km), or no
     * side is less than 25% or larger than 45% of the total length
     * (totallength >= 750km).
     */
    bool geometryok = false;

    if (d_wp < fixed(750000) && d1 >= fixed(0.28) * d_wp &&
        d2 >= fixed(0.28) * d_wp && d3 >= fixed(0.28) * d_wp)
      // small FAI
      geometryok =  true;
    else if (d_wp >= fixed(750000) &&
             d1 > d_wp / 4 && d2 > d_wp / 4 && d3 > d_wp / 4 &&
             d1 <= fixed(0.45) * d_wp && d2 <= fixed(0.45) * d_wp &&
             d3 <= fixed(0.45) * d_wp )
      // large FAI
      geometryok = true;

    if (!geometryok) {
      addValidationError(INVALID_FAI_TRIANGLE_GEOMETRY);
      valid = false;
    }
  }

  return valid;
}

void 
FAITriangleTaskFactory::update_ordered_task_behaviour(OrderedTaskBehaviour& to)
{
  FAITaskFactory::update_ordered_task_behaviour(to);
  to.min_points = 4;
  to.max_points = 4;
}
