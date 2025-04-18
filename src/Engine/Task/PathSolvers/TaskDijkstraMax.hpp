// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskDijkstra.hpp"

/**
 * Specialisation of TaskDijkstra for maximum distance search
 */
class TaskDijkstraMax final : public TaskDijkstra {
public:
  TaskDijkstraMax() noexcept
    :TaskDijkstra(false) {}

  /**
   * Search task points for targets within OZs to produce the
   * maximum-distance task.  Saves the max-distance solution
   * in the corresponding task points for later accurate distance
   * measurement.
   *
   * @return True if succeeded
   */
  bool DistanceMax() noexcept;
};
