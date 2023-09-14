// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;
class TaskManagerDialog;
class OrderedTask;

enum class WeGlideTaskSelection {
  USER,
  PUBLIC_DECLARED,
};

std::unique_ptr<Widget>
CreateWeGlideTasksPanel(TaskManagerDialog &dialog,
                        WeGlideTaskSelection selection,
                        std::unique_ptr<OrderedTask> &active_task,
                        bool *task_modified) noexcept;
