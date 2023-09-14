// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MixedTaskFactory.hpp"
#include "Constraints.hpp"

static constexpr TaskFactoryConstraints mixed_constraints = {
  true,
  false,
  false,
  false,
  true,
  2, 10,
};

static constexpr LegalPointSet mixed_start_types{
  TaskPointFactoryType::START_LINE,
  TaskPointFactoryType::START_CYLINDER,
  TaskPointFactoryType::START_BGA,
  TaskPointFactoryType::START_SECTOR,
};

static constexpr LegalPointSet mixed_im_types{
  TaskPointFactoryType::FAI_SECTOR,
  TaskPointFactoryType::AST_CYLINDER,
  TaskPointFactoryType::AAT_CYLINDER,
  TaskPointFactoryType::AAT_SEGMENT,
  TaskPointFactoryType::AAT_ANNULAR_SECTOR,
  TaskPointFactoryType::CUSTOM_KEYHOLE,
  TaskPointFactoryType::DAEC_KEYHOLE,
  TaskPointFactoryType::BGAFIXEDCOURSE_SECTOR,
  TaskPointFactoryType::BGAENHANCEDOPTION_SECTOR,
};

static constexpr LegalPointSet mixed_finish_types{
  TaskPointFactoryType::FINISH_SECTOR,
  TaskPointFactoryType::FINISH_LINE,
  TaskPointFactoryType::FINISH_CYLINDER,
};

MixedTaskFactory::MixedTaskFactory(OrderedTask& _task,
                                   const TaskBehaviour &tb)
  :AbstractTaskFactory(mixed_constraints, _task, tb,
                       mixed_start_types, mixed_im_types, mixed_finish_types)
{
}
