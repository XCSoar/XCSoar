// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskFile.hpp"

class TaskFileXCSoar: public TaskFile
{
public:
  using TaskFile::TaskFile;

  std::vector<std::string> GetList() const override {
    return {{}};
  }

  std::unique_ptr<OrderedTask> GetTask(const TaskBehaviour &task_behaviour,
                                       const Waypoints *waypoints,
                                       unsigned index) const override;
};
