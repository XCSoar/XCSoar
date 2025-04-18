// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "OrderedTaskPoint.hpp"
#include "Task/Ordered/FinishConstraints.hpp"

struct FinishConstraints;

/**
 * A FinishPoint is an abstract OrderedTaskPoint,
 * can manage finish transitions
 * but does not yet have an observation zone.
 * No taskpoints shall be present following a FinishPoint.
 *
 * Entry requires previous point to have entered to prevent spurious crossing.
 */
class FinishPoint final : public OrderedTaskPoint
{
  double safety_height;

  /**
   * A copy of OrderedTaskSettings::finish_constraints, managed by
   * SetOrderedTaskSettings().
   */
  FinishConstraints constraints;

  double fai_finish_height = 0;

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
  FinishPoint(std::unique_ptr<ObservationZonePoint> &&_oz, WaypointPtr &&wp,
              const TaskBehaviour &tb,
              const FinishConstraints &constraints,
              bool boundary_scored=false);

  /**
   * Set FAI finish height
   *
   * @param height FAI finish height (m)
   */
  void SetFaiFinishHeight(double height);

  /* virtual methods from class TaskPoint */
  double GetElevation() const noexcept override;

  /* virtual methods from class ScoredTaskPoint */
  void Reset() noexcept override;
  bool CheckEnterTransition(const AircraftState &ref_now,
                            const AircraftState &ref_last) const noexcept override;

  /* virtual methods from class OrderedTaskPoint */
  void SetTaskBehaviour(const TaskBehaviour &tb) noexcept override;
  void SetOrderedTaskSettings(const OrderedTaskSettings &otb) noexcept override;
  void SetNeighbours(OrderedTaskPoint *prev,
                     OrderedTaskPoint *next) noexcept override;
  bool IsInSector(const AircraftState &ref) const noexcept override;

private:
  /* virtual methods from class ScoredTaskPoint */
  bool EntryPrecondition() const noexcept override;
  bool ScoreFirstEntry() const noexcept override {
    return true;
  }

  /**
   * called by isInSector
   * @param state
   * @return true if state is within valid height for sector
   */
  bool InInHeightLimit(const AircraftState &state) const;
};
