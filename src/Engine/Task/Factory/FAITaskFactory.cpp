// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FAITaskFactory.hpp"
#include "Constraints.hpp"
#include "Task/Ordered/Settings.hpp"
#include "util/Compiler.h"

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
                               OrderedTask &_task,
                               const TaskBehaviour &tb) noexcept
  :AbstractTaskFactory(_constraints, _task, tb,
                       fai_start_types, fai_im_types, fai_finish_types)
{
}

FAITaskFactory::FAITaskFactory(OrderedTask &_task,
                               const TaskBehaviour &tb) noexcept
  :AbstractTaskFactory(fai_constraints, _task, tb,
                       fai_start_types, fai_im_types, fai_finish_types)
{
}

TaskValidationErrorSet
FAITaskFactory::Validate() const noexcept
{
  auto errors = AbstractTaskFactory::Validate();

  if (!IsUnique()) {
    errors |= TaskValidationErrorType::TURNPOINTS_NOT_UNIQUE;
    // warning only
  }
  return errors;
}

void 
FAITaskFactory::UpdateOrderedTaskSettings(OrderedTaskSettings &to) noexcept
{
  AbstractTaskFactory::UpdateOrderedTaskSettings(to);

  to.start_constraints.max_speed = 0;
  to.start_constraints.max_height = 0;
  to.start_constraints.max_height_ref = AltitudeReference::AGL;
  to.finish_constraints.min_height = 0;
}

TaskPointFactoryType
FAITaskFactory::GetMutatedPointType(const OrderedTaskPoint &tp) const noexcept
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
  case TaskPointFactoryType::CUSTOM_KEYHOLE:
  case TaskPointFactoryType::DAEC_KEYHOLE:
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
FAITaskFactory::GetPointDefaultSizes([[maybe_unused]] const TaskPointFactoryType type,
                                     double &start_radius,
                                     double &turnpoint_radius,
                                     double &finish_radius) const noexcept
{
  turnpoint_radius = 500;
  start_radius = finish_radius = 1000;
}
