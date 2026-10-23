// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class OrderedTask;
class TaskManagerDialog;
class Widget;

std::unique_ptr<Widget>
CreateSoaringScoringTasksPanel(TaskManagerDialog &dialog,
                               std::unique_ptr<OrderedTask> &active_task,
                               bool *task_modified) noexcept;
