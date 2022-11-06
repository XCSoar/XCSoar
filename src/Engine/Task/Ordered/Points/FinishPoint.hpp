/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
  void Reset() override;
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
  bool EntryPrecondition() const override;
  bool ScoreFirstEntry() const override {
    return true;
  }

  /**
   * called by isInSector
   * @param state
   * @return true if state is within valid height for sector
   */
  bool InInHeightLimit(const AircraftState &state) const;
};
