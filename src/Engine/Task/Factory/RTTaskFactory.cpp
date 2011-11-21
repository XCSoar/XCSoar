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
#include "Util/Macros.hpp"

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t rt_start_types[] = {
  AbstractTaskFactory::START_LINE,
  AbstractTaskFactory::START_CYLINDER,
  AbstractTaskFactory::START_SECTOR,
  AbstractTaskFactory::START_BGA,
};

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t rt_im_types[] = {
  AbstractTaskFactory::AST_CYLINDER,
  AbstractTaskFactory::KEYHOLE_SECTOR,
  AbstractTaskFactory::BGAFIXEDCOURSE_SECTOR,
  AbstractTaskFactory::BGAENHANCEDOPTION_SECTOR,
  AbstractTaskFactory::FAI_SECTOR,
};

static gcc_constexpr_data AbstractTaskFactory::LegalPointType_t rt_finish_types[] = {
  AbstractTaskFactory::FINISH_LINE,
  AbstractTaskFactory::FINISH_CYLINDER,
  AbstractTaskFactory::FINISH_SECTOR,
};

RTTaskFactory::RTTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb)
  :AbstractTaskFactory(_task, tb,
                       LegalPointConstArray(rt_start_types,
                                            ARRAY_SIZE(rt_start_types)),
                       LegalPointConstArray(rt_im_types,
                                            ARRAY_SIZE(rt_im_types)),
                       LegalPointConstArray(rt_finish_types,
                                            ARRAY_SIZE(rt_finish_types)))
{
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
  to.homogeneous_tps = false;
  to.is_closed = false;
  to.min_points = 2;
  to.max_points = 13;
  to.start_requires_arm = true;
}

AbstractTaskFactory::LegalPointType_t
RTTaskFactory::getMutatedPointType(const OrderedTaskPoint &tp) const
{
  const LegalPointType_t oldtype = getType(tp);
  LegalPointType_t newtype = oldtype;

  switch (oldtype) {

  case START_SECTOR:
  case START_LINE:
  case START_CYLINDER:
  case START_BGA:
    break;

  case KEYHOLE_SECTOR:
  case BGAFIXEDCOURSE_SECTOR:
  case BGAENHANCEDOPTION_SECTOR:
  case FAI_SECTOR:
  case AST_CYLINDER:
    break;

  case FINISH_SECTOR:
  case FINISH_LINE:
  case FINISH_CYLINDER:
    break;

  case AAT_SEGMENT:
  case AAT_ANNULAR_SECTOR:
  case AAT_CYLINDER:
      newtype = AST_CYLINDER;
    break;
  }

  return newtype;
}

