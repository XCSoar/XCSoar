// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task/TaskFile.hpp"
#include "Task/TaskFileXCSoar.hpp"
#include "Task/TaskFileSeeYou.hpp"
#include "Task/TaskFileIGC.hpp"
#include "XCTrackTaskFile.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"

#include <stdlib.h>

std::unique_ptr<TaskFile>
TaskFile::Create(Path path)
{
  // If XCSoar task file -> return new TaskFileXCSoar
  if (path.EndsWithIgnoreCase(".tsk"))
    return std::make_unique<TaskFileXCSoar>(path);

  // If SeeYou task file -> return new TaskFileSeeYou
  if (path.EndsWithIgnoreCase(".cup"))
    return std::make_unique<TaskFileSeeYou>(path);

  // If IGC file -> return new TaskFileIGC
  if (path.EndsWithIgnoreCase(".igc"))
    return std::make_unique<TaskFileIGC>(path);

  /* TODO ".xctsk" is not a real filename suffix; there is just the
     MIME type "application/xctsk" */
  if (path.EndsWithIgnoreCase(".xctsk"))
    return std::make_unique<XCTrackTaskFile>(path);

  // unknown task file type
  return nullptr;
}

std::unique_ptr<OrderedTask>
TaskFile::GetTask(Path path, const TaskBehaviour &task_behaviour,
                  const Waypoints *waypoints, unsigned index)
{
  auto file = TaskFile::Create(path);
  if (!file)
    return nullptr;

  return file->GetTask(task_behaviour, waypoints, index);
}
