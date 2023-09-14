// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "OrderedTaskPoint.hpp"

/**
 * An IntermediatePoint is an abstract OrderedTaskPoint,
 * does not yet have an observation zone, nor defines
 * how scoring is performed within.
 * All IntermediatePoints shall have a preceding and following
 * taskpoint.
 */
class IntermediateTaskPoint: 
  public OrderedTaskPoint 
{
  double safety_height;

public:    
  /**
   * Constructor.
   *
   * @param _oz Observation zone attached to this point
   * @param wp Waypoint origin of turnpoint
   * @param tb TaskBehaviour defining options (esp safety heights)
   * @param b_scored Whether distance within OZ is scored
   *
   * @return Partially-initialised object
   */
  IntermediateTaskPoint(TaskPointType _type,
                        std::unique_ptr<ObservationZonePoint> &&_oz,
                        WaypointPtr &&wp,
                        const TaskBehaviour &tb,
                        const bool b_scored = false);

  [[gnu::pure]]
  bool valid() const {
    return GetPrevious() != NULL && GetNext() != NULL;
  }

  /* virtual methods from class TaskPoint */
  double GetElevation() const noexcept override;

  /* virtual methods from class OrderedTaskPoint */
  void SetTaskBehaviour(const TaskBehaviour &tb) noexcept override;
};
