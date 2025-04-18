// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProtectedTaskManager.hpp"
#include "DefaultTask.hpp"
#include "SaveFile.hpp"
#include "LocalPath.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "system/Path.hpp"

void
ProtectedTaskManager::TaskSave(Path path)
{
  std::unique_ptr<OrderedTask> task(TaskClone());
  SaveTask(path, *task);
}

void
ProtectedTaskManager::TaskSaveDefault()
{
  const auto path = LocalPath(default_task_path);
  TaskSave(path);
}
