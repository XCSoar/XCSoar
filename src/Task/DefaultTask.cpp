// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DefaultTask.hpp"
#include "LoadFile.hpp"
#include "LocalPath.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "system/Path.hpp"

std::unique_ptr<OrderedTask>
LoadDefaultTask(const TaskBehaviour &task_behaviour,
                const Waypoints *waypoints) noexcept
{
  const auto path = LocalPath(default_task_path);

  try {
    return LoadTask(path, task_behaviour, waypoints);
  } catch (...) {
    auto task = std::make_unique<OrderedTask>(task_behaviour);
    assert(task);
    task->SetFactory(task_behaviour.task_type_default);
    return task;
  }
}
