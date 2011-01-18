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

#include "RTTaskFactory.hpp"
#include "Task/Tasks/OrderedTask.hpp"

RTTaskFactory::RTTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb):
  AbstractTaskFactory(_task, tb)
{
  m_start_types.push_back(START_LINE);
  m_start_types.push_back(START_CYLINDER);
  m_intermediate_types.push_back(AST_CYLINDER);
  m_intermediate_types.push_back(KEYHOLE_SECTOR);
  m_intermediate_types.push_back(BGAFIXEDCOURSE_SECTOR);
  m_intermediate_types.push_back(BGAENHANCEDOPTION_SECTOR);
  m_finish_types.push_back(FINISH_LINE);
  m_finish_types.push_back(FINISH_CYLINDER);
}

bool 
RTTaskFactory::validate()
{
  bool valid = AbstractTaskFactory::validate();

  return valid;
}

void 
RTTaskFactory::update_ordered_task_behaviour(OrderedTaskBehaviour& to)
{
  to.task_scored = true;
  to.fai_finish = false;  
  to.homogeneous_tps = true;
  to.is_closed = false;
  to.min_points = 3;
  to.max_points = 10;
  to.start_requires_arm = true;
}
