// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskFile.hpp"

/**
 * A class that reads and parses a SeeYou task file to an XCSoar internal
 * task representation.
 */

class TaskFileSeeYou: public TaskFile
{
public:
  using TaskFile::TaskFile;

  std::vector<std::string> GetList() const override;
  std::unique_ptr<OrderedTask> GetTask(const TaskBehaviour &task_behaviour,
                                       const Waypoints *waypoints,
                                       unsigned index) const override;
};
