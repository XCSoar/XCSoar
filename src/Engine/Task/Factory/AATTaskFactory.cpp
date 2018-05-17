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

#include "AATTaskFactory.hpp"
#include "Constraints.hpp"

static constexpr TaskFactoryConstraints aat_constraints = {
  true,
  false,
  false,
  false,
  true,
  2, 13,
};

static constexpr LegalPointSet aat_start_types{
  TaskPointFactoryType::START_LINE,
  TaskPointFactoryType::START_CYLINDER,
  TaskPointFactoryType::START_SECTOR,
  TaskPointFactoryType::START_BGA,
};

static constexpr LegalPointSet aat_im_types{
  TaskPointFactoryType::AAT_CYLINDER,
  TaskPointFactoryType::AAT_SEGMENT,
  TaskPointFactoryType::AAT_ANNULAR_SECTOR,
  TaskPointFactoryType::AAT_KEYHOLE,
};

static constexpr LegalPointSet aat_finish_types{
  TaskPointFactoryType::FINISH_LINE,
  TaskPointFactoryType::FINISH_CYLINDER,
  TaskPointFactoryType::FINISH_SECTOR,
};

AATTaskFactory::AATTaskFactory(OrderedTask& _task, const TaskBehaviour &tb)
  :AbstractTaskFactory(aat_constraints, _task, tb,
                       aat_start_types, aat_im_types, aat_finish_types)
{
}

TaskPointFactoryType
AATTaskFactory::GetMutatedPointType(const OrderedTaskPoint &tp) const
{
  const TaskPointFactoryType oldtype = GetType(tp);
  TaskPointFactoryType newtype = oldtype;

  switch (oldtype) {
  case TaskPointFactoryType::START_SECTOR:
  case TaskPointFactoryType::START_LINE:
  case TaskPointFactoryType::START_CYLINDER:
  case TaskPointFactoryType::START_BGA:
    break;

  case TaskPointFactoryType::KEYHOLE_SECTOR:
  case TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR:
  case TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR:
    newtype = AbstractTaskFactory::GetMutatedPointType(tp);
    break;

  case TaskPointFactoryType::FINISH_SECTOR:
  case TaskPointFactoryType::FINISH_LINE:
  case TaskPointFactoryType::FINISH_CYLINDER:
    break;

  case TaskPointFactoryType::FAI_SECTOR:
  case TaskPointFactoryType::SYMMETRIC_QUADRANT:
    newtype = TaskPointFactoryType::AAT_CYLINDER;
    //ToDo: create a 90 degree symmetric AAT sector
    break;

  case TaskPointFactoryType::AST_CYLINDER:
  case TaskPointFactoryType::MAT_CYLINDER:
    newtype = TaskPointFactoryType::AAT_CYLINDER;
    break;

  case TaskPointFactoryType::AAT_SEGMENT:
  case TaskPointFactoryType::AAT_CYLINDER:
  case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
  case TaskPointFactoryType::AAT_KEYHOLE:
    break;

  case TaskPointFactoryType::COUNT:
    gcc_unreachable();
  }

  return newtype;
}
