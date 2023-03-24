// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Path;
class OrderedTask;
class Waypoints;
struct TaskBehaviour;

/**
 * Throws on error.
 */
std::unique_ptr<OrderedTask>
LoadTask(Path path, const TaskBehaviour &task_behaviour,
         const Waypoints *waypoints=nullptr);
