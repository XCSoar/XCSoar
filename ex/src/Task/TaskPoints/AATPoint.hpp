/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef AATPOINT_HPP
#define AATPOINT_HPP

#include "Task/Tasks/BaseTask/IntermediatePoint.hpp"

/**
 * An AATPoint is an abstract IntermediatePoint,
 * can manage a target within the observation zone
 * but does not yet have an observation zone.
 *
 * \todo
 * - Target locking is currently not implemented.
 * - TaskBehaviour is not yet used to define how targets float.
 * - Elevation may vary with target shift
 */
class AATPoint : public IntermediatePoint {
public:
/** 
 * Constructor.  Initialises to unlocked target, target is
 * initially set to origin.
 * 
 * @param _oz Observation zone for this task point
 * @param tp Global projection 
 * @param wp Waypoint origin of turnpoint
 * @param tb Task Behaviour defining options (esp safety heights)
 * 
 * @return Partially-initialised object
 */
  AATPoint(ObservationZonePoint* _oz,
           const TaskProjection& tp,
           const Waypoint & wp,
           const TaskBehaviour &tb) : 
    IntermediatePoint(_oz,tp,wp,tb,true), 
    m_target_location(wp.Location),
    m_target_save(wp.Location),
    m_target_locked(false)
    {
    }

/** 
 * Retrieve location to be used for remaining task
 * 
 * @return Location 
 */
  const GEOPOINT& get_location_remaining() const;
  
/** 
 * Update sample, specialisation to move target for active
 * task point based on task behaviour rules.
 * Only does the target move checks when this task point
 * is currently active.
 *
 * @param state Aircraft state
 * @param task_events Callback class for feedback
 * 
 * @return True if internal state changed
 */
  bool update_sample(const AIRCRAFT_STATE& state,
                     const TaskEvents &task_events);

/**
 * Save local copy of target in case optimisation fails
 */
    void target_save() {
        m_target_save = m_target_location;
    }
/**
 * Set target from local copy
 */
    void target_restore() {
        m_target_location = m_target_save;
    }

/** 
 * Set target location explicitly
 * 
 * @param loc Location of new target
 */
  void set_target(const GEOPOINT &loc);

/** 
 * Accessor to get target location
 * 
 * @return Target location
 */
  const GEOPOINT &get_location_target() const {
    return m_target_location;
  }

/** 
 * Set target to parametric value between min and max locations.
 * Targets are only moved for current or after taskpoints, unless
 * force_if_current is true.
 * 
 * @param p Parametric range (0:1) to set target
 * @param force_if_current If current active, force range move (otherwise ignored)
 *
 * @return True if target was moved
 */
  bool set_range(const double p, const bool force_if_current);

/** 
 * Test whether aircraft has travelled close to isoline of target within threshold
 * 
 * @param state Aircraft state
 * @param threshold Threshold for distance comparision (m)
 * 
 * @return True if double leg distance from state is within threshold of target
 */
  bool close_to_target(const AIRCRAFT_STATE& state, const double threshold=0) const;

#ifdef DO_PRINT
  void print(std::ostream& f, const AIRCRAFT_STATE&state, 
             const int item=0) const;
#endif

private:
  GEOPOINT m_target_location;      /**< Location of target within OZ */
  GEOPOINT m_target_save;          /**< Saved location of target within OZ */
  bool m_target_locked;            /**< Whether target can float */

/** 
 * Check whether target needs to be moved and if so, to
 * perform the move.  Makes no assumption as to whether the aircraft
 * within or outside the observation zone.
 * 
 * @param state Current aircraft state
 * 
 * @return True if target was moved
 */
  bool check_target(const AIRCRAFT_STATE& state);

/** 
 * Check whether target needs to be moved and if so, to
 * perform the move, where aircraft is inside the observation zone
 * of the current active taskpoint.
 * 
 * @param state Current aircraft state
 * 
 * @return True if target was moved
 */
  bool check_target_inside(const AIRCRAFT_STATE& state);

/** 
 * Check whether target needs to be moved and if so, to
 * perform the move, where aircraft is outside the observation zone
 * of the current active taskpoint.
 * 
 * @param state Current aircraft state
 * 
 * @return True if target was moved
 */
  bool check_target_outside(const AIRCRAFT_STATE& state);

/** 
 * Test whether a taskpoint is equivalent to this one
 * 
 * @param other Taskpoint to compare to
 * 
 * @return True if same WP, type and OZ
 */
  bool equals(const OrderedTaskPoint* other) const;

public:
  DEFINE_VISITABLE()
};

#endif
