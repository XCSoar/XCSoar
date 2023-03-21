// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "util/tstring.hpp"

#include <memory>
#include <vector>

#include <tchar.h>

struct TaskBehaviour;
class Waypoints;
class OrderedTask;

class TaskFile
{
protected:
  AllocatedPath path;

public:
  explicit TaskFile(Path _path) noexcept
    :path(_path) {}

  virtual ~TaskFile() noexcept = default;

  /**
   * Creates a TaskFile instance according to the extension
   * @param filename The filepath
   * @return TaskFile instance
   */
  static std::unique_ptr<TaskFile> Create(Path path);

  static std::unique_ptr<OrderedTask> GetTask(Path path,
                                              const TaskBehaviour &task_behaviour,
                                              const Waypoints *waypoints,
                                              unsigned index);

  /**
   * Throws on error.
   */
  virtual std::vector<tstring> GetList() const = 0;

  virtual std::unique_ptr<OrderedTask> GetTask(const TaskBehaviour &task_behaviour,
                                               const Waypoints *waypoints,
                                               unsigned index) const = 0;
};
