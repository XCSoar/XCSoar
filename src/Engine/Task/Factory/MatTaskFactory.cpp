/*
Copyright_License {

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

#include "MatTaskFactory.hpp"
#include "Constraints.hpp"

static constexpr TaskFactoryConstraints mat_constraints = {
  true,
  false,
  false,
  false,
  false,
  2, 30,
};

static constexpr LegalPointSet mat_start_types{
  TaskPointFactoryType::START_CYLINDER,
};

static constexpr LegalPointSet mat_im_types {
  TaskPointFactoryType::MAT_CYLINDER,
};

static constexpr LegalPointSet mat_finish_types{
  TaskPointFactoryType::FINISH_LINE,
  TaskPointFactoryType::FINISH_CYLINDER,
};

MatTaskFactory::MatTaskFactory(OrderedTask& _task, const TaskBehaviour &tb)
:AbstractTaskFactory(mat_constraints, _task, tb,
                     mat_start_types, mat_im_types, mat_finish_types)
{
}

TaskPointFactoryType
MatTaskFactory::GetMutatedPointType(const OrderedTaskPoint &tp) const
{
  const TaskPointFactoryType oldtype = GetType(tp);
  TaskPointFactoryType newtype = oldtype;

  switch (oldtype) {
  case TaskPointFactoryType::START_CYLINDER:
    break;

  case TaskPointFactoryType::START_LINE:
  case TaskPointFactoryType::START_BGA:
  case TaskPointFactoryType::START_SECTOR:
    newtype = TaskPointFactoryType::START_CYLINDER;
    break;

  case TaskPointFactoryType::FINISH_LINE:
  case TaskPointFactoryType::FINISH_CYLINDER:
    break;

  case TaskPointFactoryType::FINISH_SECTOR:
    newtype = TaskPointFactoryType::FINISH_CYLINDER;
    break;

  case TaskPointFactoryType::KEYHOLE_SECTOR:
  case TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR:
  case TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR:
  case TaskPointFactoryType::FAI_SECTOR:
  case TaskPointFactoryType::AST_CYLINDER:
  case TaskPointFactoryType::AAT_SEGMENT:
  case TaskPointFactoryType::AAT_CYLINDER:
  case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
  case TaskPointFactoryType::AAT_KEYHOLE:
  case TaskPointFactoryType::SYMMETRIC_QUADRANT:
    newtype = TaskPointFactoryType::MAT_CYLINDER;
    break;

  case TaskPointFactoryType::MAT_CYLINDER:
    break;

  case TaskPointFactoryType::COUNT:
    gcc_unreachable();
  }

  return newtype;
}
