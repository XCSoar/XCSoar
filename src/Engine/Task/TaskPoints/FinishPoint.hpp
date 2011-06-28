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


#ifndef FINISHPOINT_HPP
#define FINISHPOINT_HPP

#include "Task/Tasks/BaseTask/OrderedTaskPoint.hpp"

/**
 * A FinishPoint is an abstract OrderedTaskPoint,
 * can manage finish transitions
 * but does not yet have an observation zone.
 * No taskpoints shall be present following a FinishPoint.
 *
 * Entry requires previous point to have entered to prevent spurious crossing.
 *
 */
class FinishPoint : public OrderedTaskPoint {
  fixed fai_finish_height;

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
  FinishPoint(ObservationZonePoint* _oz,
              const Waypoint & wp,
              const TaskBehaviour& tb,
              const OrderedTaskBehaviour& to);

  void reset();

/** 
 * Set previous/next taskpoints in sequence.
 * Specialises base method to check next is NULL.
 * 
 * @param prev Previous task point 
 * @param next Next task point (must be null!)
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
  bool isInSector(const AIRCRAFT_STATE &ref) const;

  /** 
   * Check if aircraft has transitioned to inside sector
   * This ensures entry only when in height limits
   * 
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if aircraft now inside (and was outside)
   */
  bool check_transition_enter(const AIRCRAFT_STATE & ref_now, 
                              const AIRCRAFT_STATE & ref_last) const;

/** 
 * Retrieve elevation of taskpoint, taking into account
 * rules and safety margins.
 * 
 * @return Minimum allowable elevation of finish point
 */
  fixed get_elevation() const;

  /** 
   * Set FAI finish height
   * 
   * @param height FAI finish height (m) 
   */
  void set_fai_finish_height(const fixed height);

private:
  bool score_first_entry() const {
    return true;
  }
  bool entry_precondition() const;

  bool is_in_height_limit(const AIRCRAFT_STATE &state) const;
};

#endif
