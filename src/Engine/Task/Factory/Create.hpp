// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskFactoryType.hpp"

#include <memory>

class AbstractTaskFactory;
class OrderedTask;
struct TaskBehaviour;

std::unique_ptr<AbstractTaskFactory>
CreateTaskFactory(TaskFactoryType type, OrderedTask &task,
                  const TaskBehaviour &task_behaviour) noexcept;
