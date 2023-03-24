// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MatTaskFactory.hpp"
#include "Constraints.hpp"
#include "util/Compiler.h"

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

MatTaskFactory::MatTaskFactory(OrderedTask &_task,
                               const TaskBehaviour &tb) noexcept
:AbstractTaskFactory(mat_constraints, _task, tb,
                     mat_start_types, mat_im_types, mat_finish_types)
{
}

TaskPointFactoryType
MatTaskFactory::GetMutatedPointType(const OrderedTaskPoint &tp) const noexcept
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

  case TaskPointFactoryType::CUSTOM_KEYHOLE:
  case TaskPointFactoryType::DAEC_KEYHOLE:
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
