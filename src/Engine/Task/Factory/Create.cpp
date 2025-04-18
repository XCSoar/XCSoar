// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Create.hpp"
#include "RTTaskFactory.hpp"
#include "FAITaskFactory.hpp"
#include "FAITriangleTaskFactory.hpp"
#include "FAIORTaskFactory.hpp"
#include "FAIGoalTaskFactory.hpp"
#include "AATTaskFactory.hpp"
#include "MatTaskFactory.hpp"
#include "MixedTaskFactory.hpp"
#include "TouringTaskFactory.hpp"
#include "util/Compiler.h"

std::unique_ptr<AbstractTaskFactory>
CreateTaskFactory(TaskFactoryType type, OrderedTask &task,
                  const TaskBehaviour &task_behaviour) noexcept
{
  switch (type) {
  case TaskFactoryType::RACING:
    return std::make_unique<RTTaskFactory>(task, task_behaviour);

  case TaskFactoryType::FAI_GENERAL:
    return std::make_unique<FAITaskFactory>(task, task_behaviour);

  case TaskFactoryType::FAI_TRIANGLE:
    return std::make_unique<FAITriangleTaskFactory>(task, task_behaviour);

  case TaskFactoryType::FAI_OR:
    return std::make_unique<FAIORTaskFactory>(task, task_behaviour);

  case TaskFactoryType::FAI_GOAL:
    return std::make_unique<FAIGoalTaskFactory>(task, task_behaviour);

  case TaskFactoryType::AAT:
    return std::make_unique<AATTaskFactory>(task, task_behaviour);

  case TaskFactoryType::MAT:
    return std::make_unique<MatTaskFactory>(task, task_behaviour);

  case TaskFactoryType::MIXED:
    return std::make_unique<MixedTaskFactory>(task, task_behaviour);

  case TaskFactoryType::TOURING:
    return std::make_unique<TouringTaskFactory>(task, task_behaviour);

  case TaskFactoryType::COUNT:
    gcc_unreachable();
  };

  /* not reachable */
  gcc_unreachable();
}
