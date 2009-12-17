/* Copyright_License {

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

#ifndef GOTOTASK_H
#define GOTOTASK_H

#include "UnorderedTask.hpp"
class Waypoint;

/**
 * Class providing ability to go to a single task point
 */
class GotoTask : 
  public UnorderedTask 
{
public:
  /** 
   * Base constructor.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param ta Advance mechanism used for advancable tasks
   * @param gp Global glide polar used for navigation calculations
   * 
   * @return Initialised object (with no waypoint to go to)
   */

  GotoTask(const TaskEvents &te, 
           const TaskBehaviour &tb,
           TaskAdvance &ta,
           GlidePolar &gp);
  ~GotoTask();

/** 
 * Size of task
 * 
 * @return Number of taskpoints in task
 */
  virtual unsigned task_size() const;

/** 
 * Retrieves the active task point sequence.
 * 
 * @return Index of active task point sequence
 */
  virtual TaskPoint* getActiveTaskPoint() const;

/** 
 * Set active task point index
 * 
 * @param index Desired active index of task sequence
 */
  virtual void setActiveTaskPoint(unsigned index);

/**
 * Determine whether active task point optionally shifted points to
 * a valid task point.
 *
 * @param index_offset offset (default 0)
 */
  bool validTaskPoint(const int index_offset=0) const;

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
  virtual bool update_sample(const AIRCRAFT_STATE &state_now, 
                             const bool full_update);
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
  virtual bool check_transitions(const AIRCRAFT_STATE& state_now, 
                                 const AIRCRAFT_STATE& state_last);

private:    
  TaskPoint* tp;
public:
#ifdef DO_PRINT
  virtual void print(const AIRCRAFT_STATE &state);
#endif

/** 
 * Accept a task point visitor; makes the visitor visit
 * all TaskPoint in the task
 * 
 * @param visitor Visitor to accept
 */
  void Accept(TaskPointVisitor& visitor, const bool reverse=false) const;
  DEFINE_VISITABLE()
};

#endif //GOTOTASK_H
