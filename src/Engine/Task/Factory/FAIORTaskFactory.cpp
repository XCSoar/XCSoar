// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FAIORTaskFactory.hpp"
#include "Constraints.hpp"

static constexpr TaskFactoryConstraints fai_or_constraints = {
  true,
  true,
  false,
  true,
  false,
  3, 3,
};

FAIORTaskFactory::FAIORTaskFactory(OrderedTask& _task,
                                   const TaskBehaviour &tb)
  :FAITaskFactory(fai_or_constraints, _task, tb)
{
}
