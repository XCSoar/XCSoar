// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskFileXCSoar.hpp"
#include "LoadFile.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"

#include <cassert>

std::unique_ptr<OrderedTask>
TaskFileXCSoar::GetTask(const TaskBehaviour &task_behaviour,
                        const Waypoints *waypoints, [[maybe_unused]] unsigned index) const
{
  assert(index == 0);

  try {
    return LoadTask(path, task_behaviour, waypoints);
  } catch (...) {
    // TODO: forward exception to caller
    return nullptr;
  }
}
