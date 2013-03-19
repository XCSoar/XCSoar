/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "FAITaskFactory.hpp"
#include "TaskFactoryConstraints.hpp"
#include "Task/Ordered/OrderedTaskBehaviour.hpp"
#include "Util/Macros.hpp"

static constexpr TaskFactoryConstraints fai_constraints = {
  true,
  true,
  false,
  false,
  false,
  2, 13,
};

static constexpr TaskPointFactoryType fai_start_types[] = {
  TaskPointFactoryType::START_SECTOR,
  TaskPointFactoryType::START_LINE,
};

static constexpr TaskPointFactoryType fai_im_types[] = {
  TaskPointFactoryType::FAI_SECTOR,
  TaskPointFactoryType::AST_CYLINDER,
};

static constexpr TaskPointFactoryType fai_finish_types[] = {
  TaskPointFactoryType::FINISH_SECTOR,
  TaskPointFactoryType::FINISH_LINE,
};

FAITaskFactory::FAITaskFactory(const TaskFactoryConstraints &_constraints,
                               OrderedTask& _task,
                               const TaskBehaviour &tb)
  :AbstractTaskFactory(_constraints, _task, tb,
                       LegalPointConstArray(fai_start_types,
                                            ARRAY_SIZE(fai_start_types)),
                       LegalPointConstArray(fai_im_types,
                                            ARRAY_SIZE(fai_im_types)),
                       LegalPointConstArray(fai_finish_types,
                                            ARRAY_SIZE(fai_finish_types)))
{
}

FAITaskFactory::FAITaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb)
  :AbstractTaskFactory(fai_constraints, _task, tb,
                       LegalPointConstArray(fai_start_types,
                                            ARRAY_SIZE(fai_start_types)),
                       LegalPointConstArray(fai_im_types,
                                            ARRAY_SIZE(fai_im_types)),
                       LegalPointConstArray(fai_finish_types,
                                            ARRAY_SIZE(fai_finish_types)))
{
}

bool
FAITaskFactory::Validate()
{
  bool valid = AbstractTaskFactory::Validate();

  if (!IsUnique()) {
    AddValidationError(TaskValidationErrorType::TURNPOINTS_NOT_UNIQUE);
    // warning only
  }
  return valid;
}

void 
FAITaskFactory::UpdateOrderedTaskBehaviour(OrderedTaskBehaviour& to)
{
  AbstractTaskFactory::UpdateOrderedTaskBehaviour(to);

  to.start_constraints.max_speed = fixed(0);
  to.start_constraints.max_height = 0;
  to.start_constraints.max_height_ref = AltitudeReference::AGL;
  to.finish_constraints.min_height = 0;
}

TaskPointFactoryType
FAITaskFactory::GetMutatedPointType(const OrderedTaskPoint &tp) const
{
  const TaskPointFactoryType oldtype = GetType(tp);
  TaskPointFactoryType newtype = oldtype;

  switch (oldtype) {
  case TaskPointFactoryType::START_SECTOR:
  case TaskPointFactoryType::START_LINE:
    break;

  case TaskPointFactoryType::START_CYLINDER:
  case TaskPointFactoryType::START_BGA:
    newtype = TaskPointFactoryType::START_SECTOR;
    break;

  case TaskPointFactoryType::KEYHOLE_SECTOR:
  case TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR:
  case TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR:
  case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
  case TaskPointFactoryType::AAT_SEGMENT:
    newtype = TaskPointFactoryType::FAI_SECTOR;
    break;

  case TaskPointFactoryType::FINISH_SECTOR:
  case TaskPointFactoryType::FINISH_LINE:
    break;

  case TaskPointFactoryType::FINISH_CYLINDER:
    newtype = TaskPointFactoryType::FINISH_SECTOR;
    break;

  case TaskPointFactoryType::FAI_SECTOR:
  case TaskPointFactoryType::AST_CYLINDER:
    break;

  case TaskPointFactoryType::AAT_CYLINDER:
  case TaskPointFactoryType::MAT_CYLINDER:
    newtype = TaskPointFactoryType::AST_CYLINDER;
    break;
  }

  return newtype;
}

void
FAITaskFactory::GetPointDefaultSizes(const TaskPointFactoryType type,
                                          fixed &start_radius,
                                          fixed &turnpoint_radius,
                                          fixed &finish_radius) const
{
  turnpoint_radius = fixed(500);
  start_radius = finish_radius = fixed(1000);
}
