// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "IntermediatePoint.hpp"

/**
 * An ASTPoint is an abstract IntermediatePoint,
 * in which the observation zone area is not used for
 * scored distance calculations (the aircraft merely has
 * to enter the observation zone)
 * but does not yet have an observation zone.
 */
class ASTPoint final : public IntermediateTaskPoint
{
  /**
   * If this is true, then exiting the observation zone is the goal,
   * not entering it.
   */
  bool score_exit = false;

public:
  /**
   * Constructor.
   * Ownership of oz is transferred to this object.  Note that AST boundaries are not scored.
   *
   * @param _oz Observation zone for this task point
   * @param wp Waypoint associated with this task point
   * @param tb Task Behaviour defining options (esp safety heights)
   *
   * @return Partially initialised object
   */
  ASTPoint(std::unique_ptr<ObservationZonePoint> &&_oz,
           WaypointPtr &&wp,
           const TaskBehaviour &tb,
           bool boundary_scored=false)
    :IntermediateTaskPoint(TaskPointType::AST, std::move(_oz), std::move(wp),
                           tb, boundary_scored) {}

  bool GetScoreExit() const {
    return score_exit;
  }

  void SetScoreExit(bool _score_exit) {
    score_exit = _score_exit;
  }

  /* virtual methods from OrderedTaskPoint */
  bool Equals(const OrderedTaskPoint &_other) const noexcept override;
};
