/* Copyright_License {

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

#ifndef GOTOTASK_H
#define GOTOTASK_H

#include "UnorderedTask.hpp"
#include "Waypoint/Waypoints.hpp"
#include "Compiler.h"

/**
 * Class providing ability to go to a single task point
 */
class GotoTask : 
  public UnorderedTask 
{
  friend class PrintHelper;

  TaskWaypoint* tp;
  const Waypoints &waypoints;

public:
  /** 
   * Base constructor.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param gp Global glide polar used for navigation calculations
   * @param wps Waypoints container to be scanned for takeoff
   * 
   * @return Initialised object (with no waypoint to go to)
   */

  GotoTask(TaskEvents &te, 
           const TaskBehaviour &tb,
           const GlidePolar &gp,
           const Waypoints &wps);
  ~GotoTask();

  virtual void SetTaskBehaviour(const TaskBehaviour &tb);

/** 
 * Size of task
 * 
 * @return Number of taskpoints in task
 */
  unsigned TaskSize() const;

/** 
 * Retrieves the active task point sequence.
 * 
 * @return Index of active task point sequence
 */
  TaskWaypoint* GetActiveTaskPoint() const;

/** 
 * Set active task point index
 * 
 * @param index Desired active index of task sequence
 */
  void SetActiveTaskPoint(unsigned index);

/**
 * Determine whether active task point optionally shifted points to
 * a valid task point.
 *
 * @param index_offset offset (default 0)
 */
  bool IsValidTaskPoint(const int index_offset=0) const;

/** 
 * Sets go to task point to specified waypoint. 
 * Obeys TaskBehaviour.goto_nonlandable, won't do anything
 * if destination is not landable.
 * 
 * @param wp Waypoint to Go To
 * @return True if successful
 */
  bool do_goto(const Waypoint& wp);

/** 
 * Resets (clears) the goto task
 */
  void reset();

/** 
 * Update internal states when aircraft state advances.
 * 
 * @param state_now Aircraft state at this time step
 * @param full_update Force update due to task state change
 *
 * @return True if internal state changes
 */
  bool update_sample(const AircraftState &state_now, 
                     const bool full_update);

  /**
   * When called on takeoff, creates a default goto task
   *
   * @param loc Location of takeoff point
   * @param terrain_alt Terrain height at takeoff point
   *
   * @return True if default task was created
   */
  bool takeoff_autotask(const GeoPoint& loc, const fixed terrain_alt);

protected:
/** 
 * Test whether (and how) transitioning into/out of task points should occur, typically
 * according to task_advance mechanism.  This also may call the task_event callbacks.
 * 
 * @param state_now Aircraft state at this time step
 * @param state_last Aircraft state at previous time step
 * 
 * @return True if transition occurred
 */
  bool check_transitions(const AircraftState& state_now, 
                         const AircraftState& state_last);

public:
/** 
 * Accept a task point visitor; makes the visitor visit
 * all TaskPoint in the task
 * 
 * @param visitor Visitor to accept
 * @param reverse Visit task points in reverse order 
 *
 */
  void tp_CAccept(TaskPointConstVisitor& visitor,
                  const bool reverse = false) const;

};

#endif //GOTOTASK_H
