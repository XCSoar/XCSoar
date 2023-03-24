// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Ptr.hpp"

/**
 * Appends wp to current Ordered task and activates the ordered task if
 * the current ordered task is valid.
 * If the current ordered task is invalid or empty, then
 * either creates a Goto task with the selected waypoint, or if in Goto mode
 * already, it creates an ordered task from the previous Goto point and the
 * selected waypoint.
 */
namespace MapTaskManager
{
  enum TaskEditResult {
    SUCCESS,
    UNMODIFIED,
    INVALID,
    NOTASK,
    MUTATED_TO_GOTO,
    MUTATED_FROM_GOTO,
  };

  TaskEditResult AppendToTask(WaypointPtr &&wp);
  TaskEditResult InsertInTask(WaypointPtr &&wp);

  TaskEditResult ReplaceInTask(WaypointPtr &&wp);
  TaskEditResult RemoveFromTask(const Waypoint &wp);

  /**
   * @param wp
   * @return TurnPointIndex if MODE_ORDERED and wp is in task
   * else returns -1
   */
  int GetIndexInTask(const Waypoint &wp);
};
