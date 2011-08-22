/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Task/Tasks/BaseTask/OrderedTaskPoint.hpp"
#include "Task/TaskBehaviour.hpp"

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
class StartPoint:
  public OrderedTaskPoint
{
  TaskStartMargins margins;

   /**
    * For use with multiple start points, whether enabled.
    */
  bool enabled;

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
  StartPoint(ObservationZonePoint* _oz,
             const Waypoint & wp,
             const TaskBehaviour& tb,
             const OrderedTaskBehaviour& to);

  virtual void SetTaskBehaviour(const TaskBehaviour &tb);

  /**
   * Set previous/next taskpoints in sequence.
   * Specialises base method to check prev is NULL.
   *
   * @param prev Previous task point (must be null!)
   * @param next Next task point in sequence
   */
  void set_neighbours(OrderedTaskPoint* prev,
                      OrderedTaskPoint* next);

  /**
   * Test whether aircraft is inside observation zone.
   *
   * @param ref Aircraft state to test
   *
   * @return True if aircraft is inside observation zone
   */
  bool isInSector(const AircraftState &ref) const;


  /**
   * Check if aircraft has transitioned to outside sector
   * This allows for exit through top of sector.
   *
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if aircraft now outside (and was inside)
   */
  bool check_transition_exit(const AircraftState & ref_now,
                             const AircraftState & ref_last) const;

  /**
   * Update sample, specialisation to check start speed/height
   *
   * @param state Aircraft state
   * @param task_events Callback class for feedback
   *
   * @return True if internal state changed
   */
  bool update_sample_near(const AircraftState& state,
                          TaskEvents &task_events,
                          const TaskProjection &projection);

  /**
   * Retrieve elevation of taskpoint, taking into account
   * rules and safety margins.
   *
   * @return Minimum allowable elevation of start point
   */
  fixed GetElevation() const;

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

private:
  bool score_last_exit() const {
    return true;
  }
};

#endif
