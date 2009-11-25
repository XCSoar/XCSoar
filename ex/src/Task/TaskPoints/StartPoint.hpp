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


#ifndef STARTPOINT_HPP
#define STARTPOINT_HPP

#include "Task/Tasks/BaseTask/OrderedTaskPoint.hpp"

/**
 * A StartPoint is an abstract OrderedTaskPoint,
 * can manage start transitions
 * but does not yet have an observation zone.
 * No taskpoints shall be present preceding a StartPoint.
 *
 * \todo
 * - currently we don't track crossing the actual line, rather it currently
 *   allows any border crossing
 * - max start height, speed
 * - gate start time?
 * - enabled/disabled for multiple start points
 */
class StartPoint : public OrderedTaskPoint {
public:
/** 
 * Constructor.  Sets task area to non-scorable; distances
 * are relative to crossing point or origin.
 * 
 * @param _oz Observation zone for this task point
 * @param tp Global projection 
 * @param wp Waypoint origin of turnpoint
 * @param tb Task Behaviour defining options (esp safety heights)
 * 
 * @return Partially-initialised object
 */
  StartPoint(ObservationZonePoint* _oz,
             const TaskProjection& tp,
             const Waypoint & wp,
             const TaskBehaviour& tb);

/** 
 * Set previous/next taskpoints in sequence.
 * Specialises base method to check prev is NULL.
 * 
 * @param prev Previous task point (must be null!)
 * @param next Next task point in sequence
 */
  virtual void set_neighbours(OrderedTaskPoint* prev,
                              OrderedTaskPoint* next);

/** 
 * Test whether aircraft has exited observation zone and
 * was previously inside; also tracks this transition and
 * clears samples inside except for last inside sample.
 * 
 * @param ref_now State current
 * @param ref_last State at last sample
 * 
 * @return True if observation zone is exited now
 */
  virtual bool transition_exit(const AIRCRAFT_STATE & ref_now, 
                               const AIRCRAFT_STATE & ref_last);

/** 
 * Update sample, specialisation to check start speed/height
 *
 * @param state Aircraft state
 * @param task_events Callback class for feedback
 * 
 * @return True if internal state changed
 */
  virtual bool update_sample(const AIRCRAFT_STATE& state,
                             const TaskEvents &task_events);

/** 
 * Retrieve elevation of taskpoint, taking into account
 * rules and safety margins. 
 * 
 * @return Minimum allowable elevation of start point
 */
  virtual double getElevation() const;

/** 
 * Test whether a taskpoint is equivalent to this one
 * 
 * @param other Taskpoint to compare to
 * 
 * @return True if same WP, type and OZ
 */
  virtual bool equals(const OrderedTaskPoint* other) const;

protected:
  bool enabled; /**< For future use with multiple start points, whether enabled */
public:
  DEFINE_VISITABLE()
};

#endif
