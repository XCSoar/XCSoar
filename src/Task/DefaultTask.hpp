// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <memory>

struct TaskBehaviour;
class OrderedTask;
class Waypoints;

#define default_task_path "default.tsk"

/**
 * Path to default.tsk for loading (tasks/ first, legacy data-root fallback).
 */
[[gnu::pure]]
AllocatedPath GetDefaultTaskPath() noexcept;

/**
 * Path to default.tsk for saving (always under tasks/).
 */
AllocatedPath GetDefaultTaskSavePath() noexcept;

/**
 * Creates an ordered task based on the default.tsk file
 * Consumer's responsibility to delete task
 *
 * @param waypoints waypoint structure
 * @param failfactory default task type used if default.tsk is invalid
 * @return OrderedTask from default.tsk file or if default.tsk is invalid
 * or non-existent, returns empty task with defaults set by
 * config task defaults
 */
std::unique_ptr<OrderedTask>
LoadDefaultTask(const TaskBehaviour &task_behaviour,
                const Waypoints *waypoints) noexcept;
