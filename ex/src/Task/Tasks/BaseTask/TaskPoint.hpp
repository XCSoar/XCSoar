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


#ifndef TASKPOINT_HPP
#define TASKPOINT_HPP

#include "Util/Serialisable.hpp"
#include "Navigation/ReferencePoint.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Util/GenericVisitor.hpp"
#include "Task/TaskBehaviour.hpp"

/**
 * Base class for all task points 
 *
 * \todo
 * - Lookup terrain height?
 */

class TaskPoint : 
  public ReferencePoint, 
  public Serialisable,
  public BaseVisitable<>
{

public:
/** 
 * Constructor.  Location and elevation of waypoint is used
 * as the task point's reference values; a copy of the waypoint
 * is also stored to facilitate user-feedback.
 * 
 * @param wp Waypoint to be used as task point origin
 * @param tb Task Behaviour defining options (esp safety heights)
 * 
 * @return Initialised object
 */
  TaskPoint(const Waypoint & wp,
            const TaskBehaviour &tb) : ReferencePoint(wp.Location),
                                       m_elevation(wp.Altitude),
                                       m_task_behaviour(tb),
                                       m_waypoint(wp)
    { }

/**
 * Destructor.  Does nothing yet.
 */
  virtual ~TaskPoint() {};

/** 
 * Retrieve location to be used for remaining task
 * (for a pure TaskPoint, this is the reference location)
 * 
 * @return Location 
 */
  virtual const GEOPOINT& get_location_remaining() const;

/** 
 * Calculate vector from aircraft to destination
 * 
 * @return Vector for task leg
 */  
  virtual const GeoVector get_vector_remaining(const AIRCRAFT_STATE &) const = 0;

/** 
 * Calculate vector from aircraft to destination
 * 
 * @return Vector for task leg
 */  
  virtual const GeoVector get_vector_planned() const = 0;

/** 
 * Calculate vector travelled along this leg
 * 
 * @return Vector for task leg
 */  
  virtual const GeoVector get_vector_travelled(const AIRCRAFT_STATE &) const = 0;

/** 
 * Dummy null method.
 * Set target to parametric value between min and max locations.
 * Targets are only moved for current or after taskpoints, unless
 * force_if_current is true.
 * 
 * @param p Parametric range (0:1) to set target
 * @param force_if_current If current active, force range move (otherwise ignored)
 *
 * @return True if target was moved
 */
  virtual bool set_range(const double p, const bool force_if_current) {
    return false;
  };

/**
 * Save local copy of target in case optimisation fails
 */
  virtual void target_save() {};
/**
 * Restore target from local copy
 */
  virtual void target_restore() {};

/** 
 * Check whether aircraft has entered the observation zone.
 * 
 * @return True if observation zone has been entered
 */
  virtual bool has_entered() const = 0;

/** 
 * Recall aircraft state where it entered the observation zone.
 * 
 * @return State at entry, or null if never entered
 */
  virtual const AIRCRAFT_STATE& get_state_entered() const = 0;

/** 
 * Recall waypoint associated with this task point.  
 * Can be used for user feedback (e.g. queries on details of active 
 * task point)
 * 
 * @return Copy of waypoint associated with this task point
 */
  const Waypoint& get_waypoint() const {
    return m_waypoint;
  }

/** 
 * Retrieve elevation of taskpoint, taking into account
 * rules and safety margins.  
 * 
 * @return Minimum allowable elevation of task point
 */
  virtual fixed get_elevation() const = 0;

#ifdef DO_PRINT
  virtual void print(std::ostream& f, const AIRCRAFT_STATE &state) const;
#endif

protected:
  const fixed m_elevation; /**< Altitude (AMSL, m) of task point terrain */
  const TaskBehaviour &m_task_behaviour; /**< Reference to task behaviour (for options) */
private:
  const Waypoint m_waypoint; /**< local copy of waypoint */
public:
  DEFINE_VISITABLE()
};

#endif
