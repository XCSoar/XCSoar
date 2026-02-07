// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

struct TaskBehaviour;
class OrderedTask;
class Waypoints;

#define default_task_path "Default.tsk"

/**
 * Creates an ordered task based on the Default.tsk file
 * Consumer's responsibility to delete task
 *
 * @param waypoints waypoint structure
 * @param failfactory default task type used if Default.tsk is invalid
 * @return OrderedTask from Default.tsk file or if Default.tsk is invalid
 * or non-existent, returns empty task with defaults set by
 * config task defaults
 */
std::unique_ptr<OrderedTask>
LoadDefaultTask(const TaskBehaviour &task_behaviour,
                const Waypoints *waypoints) noexcept;
