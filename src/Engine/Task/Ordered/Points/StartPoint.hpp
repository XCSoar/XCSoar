/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
class StartPoint gcc_final : public OrderedTaskPoint {
  fixed safety_height;

  TaskStartMargins margins;

  /**
   * A copy of OrderedTaskBehaviour::start_constraints, managed by
   * SetOrderedTaskBehaviour().
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
             const Waypoint &wp,
             const TaskBehaviour &tb,
             const StartConstraints &constraints);

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
                       const TaskProjection &projection);

  /* virtual methods from class TaskPoint */
  virtual void SetTaskBehaviour(const TaskBehaviour &tb) gcc_override;
  virtual fixed GetElevation() const gcc_override;

  /* virtual methods from class ObservationZoneClient */
  virtual bool IsInSector(const AircraftState &ref) const gcc_override;
  virtual bool CheckExitTransition(const AircraftState &ref_now,
                                   const AircraftState &ref_last) const gcc_override;

  /* virtual methods from class SampledTaskPoint */
  virtual bool UpdateSampleNear(const AircraftState &state,
                                const TaskProjection &projection) gcc_override;

  /* virtual methods from class OrderedTaskPoint */
  virtual void SetOrderedTaskBehaviour(const OrderedTaskBehaviour &otb) gcc_override;
  virtual void SetNeighbours(OrderedTaskPoint *prev,
                             OrderedTaskPoint *next) gcc_override;

private:
  /* virtual methods from class ScoredTaskPoint */
  virtual bool ScoreLastExit() const gcc_override {
    return true;
  }
};

#endif
