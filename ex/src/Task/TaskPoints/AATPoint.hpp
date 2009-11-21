/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
    TargetLocked(false), 
    TargetLocation(wp.Location)
    {
    }

/** 
 * Retrieve location to be used for remaining task
 * 
 * @return Location 
 */
  virtual GEOPOINT get_reference_remaining() const;
  
/** 
 * Retrieve location to be used for task travelled
 * 
 * @return Location 
 */
  virtual GEOPOINT get_reference_travelled() const;
  
/** 
 * Retrieve location to be used for task scored
 * 
 * @return Location 
 */
  virtual GEOPOINT get_reference_scored() const;

/** 
 * Retrieve elevation of taskpoint, taking into account
 * rules and safety margins.  
 * 
 * \todo
 * - not implemented: elevation may vary with target shift
 *
 * @return Minimum allowable elevation of task point
 */
  virtual double getElevation() const;

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
  virtual bool update_sample(const AIRCRAFT_STATE& state,
                             const TaskEvents &task_events);

/** 
 * Set target location explicitly
 * 
 * @param loc Location of new target
 */
  virtual void set_target(const GEOPOINT &loc);

/** 
 * Accessor to get target location
 * 
 * @return Target location
 */
  const GEOPOINT &getTargetLocation() const {
    return TargetLocation;
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
  virtual bool set_range(const double p, const bool force_if_current);

/** 
 * Re-project internal data; must
 * be called when global task projection changes. 
 * 
 */
  virtual void update_projection();

#ifdef DO_PRINT
  virtual void print(std::ostream& f, const AIRCRAFT_STATE&state, 
                     const int item=0) const;
#endif

protected:
  GEOPOINT TargetLocation;      /**< Location of target within OZ */
  bool TargetLocked;            /**< Whether target can float */

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
  virtual bool equals(const OrderedTaskPoint* other) const;

public:
  DEFINE_VISITABLE()
};

#endif
