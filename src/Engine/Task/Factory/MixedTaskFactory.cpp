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

#include "MixedTaskFactory.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/OrderedTaskBehaviour.hpp"
#include "Util/Macros.hpp"

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t mixed_start_types[] = {
  AbstractTaskFactory::START_LINE,
  AbstractTaskFactory::START_CYLINDER,
  AbstractTaskFactory::START_BGA,
  AbstractTaskFactory::START_SECTOR,
};

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t mixed_im_types[] = {
  AbstractTaskFactory::FAI_SECTOR,
  AbstractTaskFactory::AST_CYLINDER,
  AbstractTaskFactory::AAT_CYLINDER,
  AbstractTaskFactory::AAT_SEGMENT,
  AbstractTaskFactory::AAT_ANNULAR_SECTOR,
  AbstractTaskFactory::KEYHOLE_SECTOR,
  AbstractTaskFactory::BGAFIXEDCOURSE_SECTOR,
  AbstractTaskFactory::BGAENHANCEDOPTION_SECTOR,
};

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t mixed_finish_types[] = {
  AbstractTaskFactory::FINISH_SECTOR,
  AbstractTaskFactory::FINISH_LINE,
  AbstractTaskFactory::FINISH_CYLINDER,
};

MixedTaskFactory::MixedTaskFactory(OrderedTask& _task,
                                   const TaskBehaviour &tb)
  :AbstractTaskFactory(_task, tb,
                       LegalPointConstArray(mixed_start_types,
                                            ARRAY_SIZE(mixed_start_types)),
                       LegalPointConstArray(mixed_im_types,
                                            ARRAY_SIZE(mixed_im_types)),
                       LegalPointConstArray(mixed_finish_types,
                                            ARRAY_SIZE(mixed_finish_types)))
{
}

void 
MixedTaskFactory::update_ordered_task_behaviour(OrderedTaskBehaviour& to)
{
  to.task_scored = true;
  to.fai_finish = false;  
  to.homogeneous_tps = false;
  to.is_closed = false;
  to.min_points = 2;
  to.max_points = 10;
  to.start_requires_arm = true;
}
