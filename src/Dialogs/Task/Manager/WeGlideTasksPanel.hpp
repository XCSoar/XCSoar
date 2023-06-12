// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;
class TaskManagerDialog;
class OrderedTask;

std::unique_ptr<Widget>
CreateWeGlideTasksPanel(TaskManagerDialog &dialog,
                        std::unique_ptr<OrderedTask> &active_task,
                        bool *task_modified) noexcept;
