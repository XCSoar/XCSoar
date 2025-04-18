// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;
class TaskManagerDialog;
class OrderedTask;

std::unique_ptr<Widget>
CreateTaskListPanel(TaskManagerDialog &dialog,
                    std::unique_ptr<OrderedTask> &active_task,
                    bool *task_modified) noexcept;

/**
 * Mark the previously scanned list of tasks as "dirty".  Before the
 * list will be shown next time, a new list will be collected.  Call
 * this function after a task file has been created or deleted.
 */
void
DirtyTaskListPanel();
