// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project


#pragma once

#include "Task/Points/TaskWaypoint.hpp"

struct TaskBehaviour;

/**
 * Class for unordered task points (e.g. goto and abort)
 *
 */
class UnorderedTaskPoint final : public TaskWaypoint {
  double safety_height_arrival;

public:
  /**
   * Constructor.
   * 
   * @param wp Waypoint to be used as task point origin
   * @param tb Task Behaviour defining options (esp safety heights)
   */
  UnorderedTaskPoint(WaypointPtr wp,
                     const TaskBehaviour &tb);

  void SetTaskBehaviour(const TaskBehaviour &tb);

  /* virtual methods from class TaskPoint */
  GeoVector GetVectorRemaining(const GeoPoint &reference) const noexcept override;
  double GetElevation() const noexcept override;
};
