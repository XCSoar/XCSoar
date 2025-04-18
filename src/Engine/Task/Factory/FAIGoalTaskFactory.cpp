// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FAIGoalTaskFactory.hpp"
#include "Constraints.hpp"

static constexpr TaskFactoryConstraints fai_goal_constraints = {
  true,
  true,
  false,
  false,
  false,
  2, 2,
};

FAIGoalTaskFactory::FAIGoalTaskFactory(OrderedTask& _task,
                               const TaskBehaviour &tb):
  FAITaskFactory(fai_goal_constraints, _task, tb)
{
}
