// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TaskPoint.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Waypoint/Ptr.hpp"

/** Task points that have a waypoint associated */
class TaskWaypoint :
  public TaskPoint
{
  WaypointPtr waypoint;

public:
  /**
   * Constructor.  Location and elevation of waypoint is used
   * as the task point's reference values; a copy of the waypoint
   * is also stored to facilitate user-feedback.
   *
   * @param wp Waypoint to be used as task point origin
   * @param tb Task Behaviour defining options (esp safety heights)
   *
   * @return Initialised object
   */
  TaskWaypoint(TaskPointType _type, WaypointPtr &&wp) noexcept
    :TaskPoint(_type, wp->location),
     waypoint(std::move(wp)) {}

  /**
   * Recall waypoint associated with this task point.
   * Can be used for user feedback (e.g. queries on details of active
   * task point)
   */
  [[gnu::pure]]
  const Waypoint &GetWaypoint() const noexcept {
    return *waypoint;
  }

  WaypointPtr GetWaypointPtr() const noexcept {
    return waypoint;
  }

protected:
  /**
   * Altitude (AMSL, m) of task point terrain.
   */
  double GetBaseElevation() const noexcept {
    // TODO can we avoid the zero fallback somehow?
    return waypoint->GetElevationOrZero();
  }
};
