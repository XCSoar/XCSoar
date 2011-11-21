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

#include "TouringTaskFactory.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/OrderedTaskBehaviour.hpp"
#include "Util/Macros.hpp"

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t touring_start_types[] = {
  AbstractTaskFactory::START_CYLINDER,
};

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t touring_im_types[] = {
  AbstractTaskFactory::FAI_SECTOR,
};

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t touring_finish_types[] = {
  AbstractTaskFactory::FINISH_CYLINDER,
};

TouringTaskFactory::TouringTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb)
  :AbstractTaskFactory(_task, tb,
                       LegalPointConstArray(touring_start_types,
                                            ARRAY_SIZE(touring_start_types)),
                       LegalPointConstArray(touring_im_types,
                                            ARRAY_SIZE(touring_im_types)),
                       LegalPointConstArray(touring_finish_types,
                                            ARRAY_SIZE(touring_finish_types)))
{
}

void 
TouringTaskFactory::update_ordered_task_behaviour(OrderedTaskBehaviour& to)
{
  to.task_scored = false;
  to.aat_min_time = fixed_zero;
  to.fai_finish = false;  
  to.homogeneous_tps = true;
  to.min_points = 2;
  to.max_points = 10;
  to.is_closed = false;

  to.start_max_speed = fixed_zero;
  to.start_max_height = 0;
  to.start_max_height_ref = hrAGL;
  to.finish_min_height = 0;
  to.start_requires_arm = false;
}
