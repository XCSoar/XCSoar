// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "OrderedTaskPoint.hpp"
#include "Task/TaskBehaviour.hpp"
#include "Task/Ordered/StartConstraints.hpp"

/**
 * A StartPoint is an abstract OrderedTaskPoint,
 * can manage start transitions
 * but does not yet have an observation zone.
 * No taskpoints shall be present preceding a StartPoint.
 *
 * \todo
 * - gate start time?
 * - enabled/disabled for multiple start points
 */
class StartPoint final : public OrderedTaskPoint {
  double safety_height;

  TaskStartMargins margins;

  /**
   * A copy of OrderedTaskSettings::start_constraints, managed by
   * SetOrderedTaskSettings().
   */
  StartConstraints constraints;

public:
  /**
   * Constructor.  Sets task area to non-scorable; distances
   * are relative to crossing point or origin.
   *
   * @param _oz Observation zone for this task point
   * @param wp Waypoint origin of turnpoint
   * @param tb Task Behaviour defining options (esp safety heights)
   * @param to OrderedTask Behaviour defining options
   *
   * @return Partially-initialised object
   */
  StartPoint(std::unique_ptr<ObservationZonePoint> &&_oz,
             WaypointPtr &&wp,
             const TaskBehaviour &tb,
             const StartConstraints &constraints);

  bool DoesRequireArm() const {
    return constraints.require_arm;
  }

  bool GetScoreExit() const noexcept {
    return constraints.score_exit;
  }

  /**
   * Search the observation zone boundary for the node which minimises
   * the distance from the aircraft via that node to the next task
   * point, and make it this task point's "remaining" location.
   *
   * This runs on every cycle while the start is the active task
   * point, wherever the aircraft is; it does not require the aircraft
   * to be inside the sector.  It does not run before takeoff, though:
   * OrderedTask::CheckTransitions() returns early while the aircraft
   * is not flying.
   *
   * @param state Current aircraft state
   * @param next Next task point following the start
   * @param projection the projection used by the task
   */
  void find_best_start(const AircraftState &state,
                       const OrderedTaskPoint &next,
                       const FlatProjection &projection);

  /* virtual methods from class TaskPoint */
  double GetElevation() const noexcept override;

  /* virtual methods from class ScoredTaskPoint */
  bool CheckExitTransition(const AircraftState &ref_now,
                           const AircraftState &ref_last) const noexcept override;

  /* virtual methods from class OrderedTaskPoint */
  void SetTaskBehaviour(const TaskBehaviour &tb) noexcept override;
  void SetOrderedTaskSettings(const OrderedTaskSettings &s) noexcept override;
  void SetNeighbours(OrderedTaskPoint *prev,
                     OrderedTaskPoint *next) noexcept override;
  bool IsInSector(const AircraftState &ref) const noexcept override;

private:
  /* virtual methods from class ScoredTaskPoint */
  bool ScoreLastExit() const noexcept override {
    return true;
  }
};
