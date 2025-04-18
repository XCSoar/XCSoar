// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;
class TaskManagerDialog;
struct TaskLook;
struct AirspaceLook;
class OrderedTask;

std::unique_ptr<Widget>
CreateTaskEditPanel(TaskManagerDialog &dialog,
                    const TaskLook &task_look,
                    const AirspaceLook &airspace_look,
                    std::unique_ptr<OrderedTask> &active_task,
                    bool *task_modified) noexcept;
