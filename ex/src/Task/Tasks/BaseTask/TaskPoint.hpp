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


#ifndef TASKPOINT_HPP
#define TASKPOINT_HPP

#include "Util/Serialisable.hpp"
#include "Navigation/ReferencePoint.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Util/GenericVisitor.hpp"
#include "Task/TaskBehaviour.hpp"

struct GlideResult;
class GlidePolar;
struct GeoVector;

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
                                       Elevation(wp.Altitude),
                                       waypoint(wp),
                                       task_behaviour(tb)
    { }

/**
 * Destructor.  Does nothing yet.
 */
  virtual ~TaskPoint() {};

/** 
 * Retrieve elevation of taskpoint, taking into account
 * rules and safety margins.  
 * 
 * @return Minimum allowable elevation of task point
 */
  virtual double getElevation() const;

/** 
 * Retrieve location to be used for remaining task
 * (for a pure TaskPoint, this is the reference location)
 * 
 * @return Location 
 */
  virtual GEOPOINT get_reference_remaining() const;

/** 
 * Calculate vector from aircraft to destination
 * 
 * @return Vector for task leg
 */  
  virtual const GeoVector get_vector_remaining(const AIRCRAFT_STATE &) const;

/** 
 * Compute optimal glide solution from aircraft to destination.
 * 
 * @param state Aircraft state at origin
 * @param polar Glide polar used for computations
 * @param minH Minimum height at destination over-ride (max of this or the task points's elevation is used)
 * @return GlideResult of task leg
 */
  GlideResult glide_solution_remaining(const AIRCRAFT_STATE &state, 
                                        const GlidePolar &polar,
                                        const double minH=0) const;

/** 
 * Compute optimal glide solution from aircraft to destination, with
 * externally supplied sink rate.  This is used to calculate the sink
 * rate required for glide-only solutions.
 * 
 * @param state Aircraft state at origin
 * @param polar Glide polar used for computations
 * @param S Sink rate (m/s, positive down)
 * @return GlideResult of task leg
 */
  GlideResult glide_solution_sink(const AIRCRAFT_STATE &state, 
                                   const GlidePolar &polar,
                                   const double S) const;


/** 
 * Compute optimal glide solution from previous point to aircraft towards destination.
 * (For pure TaskPoints, this is null)
 * 
 * @param state Aircraft state
 * @param polar Glide polar used for computations
 * @param minH Minimum height at destination over-ride (max of this or the task points's elevation is used)
 * @return GlideResult of task leg
 */
  virtual GlideResult glide_solution_travelled(const AIRCRAFT_STATE &state, 
                                                const GlidePolar &polar,
                                                const double minH=0) const;

/** 
 * Compute optimal glide solution from aircraft to destination, or modified
 * destination (e.g. where specialised TaskPoint has a target)
 * 
 * @param state Aircraft state at origin
 * @param polar Glide polar used for computations
 * @param minH Minimum height at destination over-ride (max of this or the task points's elevation is used)
 * @return GlideResult of task leg
 */
  virtual GlideResult glide_solution_planned(const AIRCRAFT_STATE &state, 
                                              const GlidePolar &polar,
                                              const double minH=0) const;

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
 * Dummy method (always false for pure TaskPoints)
 * Check whether aircraft has entered the observation zone.
 * 
 * @return True if observation zone has been entered
 */
  virtual bool has_entered() const {
    return false;
  }

/** 
 * Dummy method (always null for pure TaskPoints)
 * Recall aircraft state where it entered the observation zone.
 * 
 * @return State at entry, or null if never entered
 */
  virtual AIRCRAFT_STATE get_state_entered() const {
    // this should never get called
    AIRCRAFT_STATE null_state;
    return null_state;
  }

/** 
 * Recall waypoint associated with this task point.  
 * Can be used for user feedback (e.g. queries on details of active 
 * task point)
 * 
 * @return Copy of waypoint associated with this task point
 */
  const Waypoint& get_waypoint() const {
    return waypoint;
  }

#ifdef DO_PRINT
  virtual void print(std::ostream& f, const AIRCRAFT_STATE &state) const;
#endif

protected:
  const Waypoint waypoint; /**< local copy of waypoint */
  const double Elevation; /**< Altitude (AMSL, m) of task point terrain */
  const TaskBehaviour &task_behaviour; /**< Reference to task behaviour (for options) */
public:
  DEFINE_VISITABLE()
};

#endif
