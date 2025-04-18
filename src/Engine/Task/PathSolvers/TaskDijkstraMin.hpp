// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskDijkstra.hpp"

/**
 * Specialisation of TaskDijkstra for minimum distance search
 */
class TaskDijkstraMin final : public TaskDijkstra {
public:
  TaskDijkstraMin() noexcept
    :TaskDijkstra(true) {}

  /**
   * Search task points for targets within OZs to produce the
   * minimum-distance task.  Saves the minimum-distance solution
   * in the corresponding task points for later accurate distance
   * measurement.
   *
   * Note that the minimum distance task is the minimum distance
   * remaining and is therefore sensitive to the specified aircraft
   * location.
   *
   * @param location Location of aircraft
   * @return True if succeeded
   */
  bool DistanceMin(const SearchPoint &location) noexcept;
};
