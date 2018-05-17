/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Constraints.hpp"
#include "Task/Ordered/Settings.hpp"

static constexpr TaskFactoryConstraints fai_constraints = {
  true,
  true,
  false,
  false,
  false,
  2, 13,
};

static constexpr LegalPointSet fai_start_types{
  TaskPointFactoryType::START_SECTOR,
  TaskPointFactoryType::START_LINE,
};

static constexpr LegalPointSet fai_im_types{
  TaskPointFactoryType::FAI_SECTOR,
  TaskPointFactoryType::AST_CYLINDER,
};

static constexpr LegalPointSet fai_finish_types{
  TaskPointFactoryType::FINISH_SECTOR,
  TaskPointFactoryType::FINISH_LINE,
};

FAITaskFactory::FAITaskFactory(const TaskFactoryConstraints &_constraints,
                               OrderedTask& _task,
                               const TaskBehaviour &tb)
  :AbstractTaskFactory(_constraints, _task, tb,
                       fai_start_types, fai_im_types, fai_finish_types)
{
}

FAITaskFactory::FAITaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb)
  :AbstractTaskFactory(fai_constraints, _task, tb,
                       fai_start_types, fai_im_types, fai_finish_types)
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
FAITaskFactory::UpdateOrderedTaskSettings(OrderedTaskSettings& to)
{
  AbstractTaskFactory::UpdateOrderedTaskSettings(to);

  to.start_constraints.max_speed = 0;
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

  case TaskPointFactoryType::AAT_KEYHOLE:
  case TaskPointFactoryType::KEYHOLE_SECTOR:
  case TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR:
  case TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR:
  case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
  case TaskPointFactoryType::AAT_SEGMENT:
  case TaskPointFactoryType::SYMMETRIC_QUADRANT:
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

  case TaskPointFactoryType::COUNT:
    gcc_unreachable();
  }

  return newtype;
}

void
FAITaskFactory::GetPointDefaultSizes(const TaskPointFactoryType type,
                                     double &start_radius,
                                     double &turnpoint_radius,
                                     double &finish_radius) const
{
  turnpoint_radius = 500;
  start_radius = finish_radius = 1000;
}
