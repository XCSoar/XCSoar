// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AATTaskFactory.hpp"
#include "Constraints.hpp"
#include "util/Compiler.h"

static constexpr TaskFactoryConstraints aat_constraints = {
  true,
  false,
  false,
  false,
  false,  // Arm start manually
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

AATTaskFactory::AATTaskFactory(OrderedTask &_task,
                               const TaskBehaviour &tb) noexcept
  :AbstractTaskFactory(aat_constraints, _task, tb,
                       aat_start_types, aat_im_types, aat_finish_types)
{
}

TaskPointFactoryType
AATTaskFactory::GetMutatedPointType(const OrderedTaskPoint &tp) const noexcept
{
  const TaskPointFactoryType oldtype = GetType(tp);
  TaskPointFactoryType newtype = oldtype;

  switch (oldtype) {
  case TaskPointFactoryType::START_SECTOR:
  case TaskPointFactoryType::START_LINE:
  case TaskPointFactoryType::START_CYLINDER:
  case TaskPointFactoryType::START_BGA:
    break;

  case TaskPointFactoryType::CUSTOM_KEYHOLE:
  case TaskPointFactoryType::DAEC_KEYHOLE:
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
