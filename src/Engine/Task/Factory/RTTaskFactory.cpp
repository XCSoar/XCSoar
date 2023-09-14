// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RTTaskFactory.hpp"
#include "Constraints.hpp"
#include "util/Compiler.h"

static constexpr TaskFactoryConstraints rt_constraints = {
  true,
  false,
  false,
  false,
  false,
  2, 30,
};

static constexpr LegalPointSet rt_start_types{
  TaskPointFactoryType::START_LINE,
  TaskPointFactoryType::START_CYLINDER,
  TaskPointFactoryType::START_SECTOR,
  TaskPointFactoryType::START_BGA,
};

static constexpr LegalPointSet rt_im_types{
  TaskPointFactoryType::AST_CYLINDER,
  TaskPointFactoryType::CUSTOM_KEYHOLE,
  TaskPointFactoryType::DAEC_KEYHOLE,
  TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR,
  TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR,
  TaskPointFactoryType::FAI_SECTOR,
  TaskPointFactoryType::SYMMETRIC_QUADRANT,
};

static constexpr LegalPointSet rt_finish_types{
  TaskPointFactoryType::FINISH_LINE,
  TaskPointFactoryType::FINISH_CYLINDER,
  TaskPointFactoryType::FINISH_SECTOR,
};

RTTaskFactory::RTTaskFactory(OrderedTask &_task,
                             const TaskBehaviour &tb) noexcept
  :AbstractTaskFactory(rt_constraints, _task, tb,
                       rt_start_types, rt_im_types, rt_finish_types)
{
}

TaskPointFactoryType
RTTaskFactory::GetMutatedPointType(const OrderedTaskPoint &tp) const noexcept
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
  case TaskPointFactoryType::FAI_SECTOR:
  case TaskPointFactoryType::AST_CYLINDER:
  case TaskPointFactoryType::SYMMETRIC_QUADRANT:
    break;

  case TaskPointFactoryType::FINISH_SECTOR:
  case TaskPointFactoryType::FINISH_LINE:
  case TaskPointFactoryType::FINISH_CYLINDER:
    break;

  case TaskPointFactoryType::AAT_KEYHOLE:
    newtype = TaskPointFactoryType::CUSTOM_KEYHOLE;
    break;

  case TaskPointFactoryType::AAT_SEGMENT:
  case TaskPointFactoryType::AAT_ANNULAR_SECTOR:
  case TaskPointFactoryType::MAT_CYLINDER:
  case TaskPointFactoryType::AAT_CYLINDER:
    newtype = TaskPointFactoryType::AST_CYLINDER;
    break;

  case TaskPointFactoryType::COUNT:
    gcc_unreachable();
  }

  return newtype;
}

