/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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


#ifndef STARTPOINT_HPP
#define STARTPOINT_HPP

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
  StartPoint(ObservationZonePoint *_oz,
             WaypointPtr &&wp,
             const TaskBehaviour &tb,
             const StartConstraints &constraints);

  bool DoesRequireArm() const {
    return constraints.require_arm;
  }

  /**
   * Search for the min point on the boundary from
   * the aircraft state to the next point.  Should only
   * be performed when the aircraft state is inside the sector
   *
   * @param state Current aircraft state
   * @param next Next task point following the start
   */
  void find_best_start(const AircraftState &state,
                       const OrderedTaskPoint &next,
                       const FlatProjection &projection);

  /* virtual methods from class TaskPoint */
  double GetElevation() const override;

  /* virtual methods from class ScoredTaskPoint */
  bool CheckExitTransition(const AircraftState &ref_now,
                           const AircraftState &ref_last) const override;

  /* virtual methods from class OrderedTaskPoint */
  void SetTaskBehaviour(const TaskBehaviour &tb) override;
  void SetOrderedTaskSettings(const OrderedTaskSettings &s) override;
  void SetNeighbours(OrderedTaskPoint *prev,
                     OrderedTaskPoint *next) override;
  bool IsInSector(const AircraftState &ref) const override;

private:
  /* virtual methods from class ScoredTaskPoint */
  bool ScoreLastExit() const override {
    return true;
  }
};

#endif
