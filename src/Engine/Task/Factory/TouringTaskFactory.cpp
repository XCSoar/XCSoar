// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TouringTaskFactory.hpp"
#include "Constraints.hpp"
#include "Task/Ordered/Settings.hpp"

static constexpr TaskFactoryConstraints touring_constraints = {
  false,
  false,
  true,
  false,
  false,
  2, 10,
};

static constexpr LegalPointSet touring_start_types{
  TaskPointFactoryType::START_CYLINDER,
};

static constexpr LegalPointSet touring_im_types{
  TaskPointFactoryType::FAI_SECTOR,
};

static constexpr LegalPointSet touring_finish_types{
  TaskPointFactoryType::FINISH_CYLINDER,
};

TouringTaskFactory::TouringTaskFactory(OrderedTask &_task,
                                       const TaskBehaviour &tb) noexcept
  :AbstractTaskFactory(touring_constraints, _task, tb,
                       touring_start_types, touring_im_types,
                       touring_finish_types)
{
}

void 
TouringTaskFactory::UpdateOrderedTaskSettings(OrderedTaskSettings &to) noexcept
{
  AbstractTaskFactory::UpdateOrderedTaskSettings(to);

  to.aat_min_time = {};

  to.start_constraints.max_speed = 0;
  to.start_constraints.max_height = 0;
  to.start_constraints.max_height_ref = AltitudeReference::AGL;
  to.finish_constraints.min_height = 0;
}
