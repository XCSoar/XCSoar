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

#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include "Compiler.h"
#include "Util/NonCopyable.hpp"
#include "Tasks/TaskInterface.hpp"
#include "Tasks/AbortTask.hpp"
#include "Tasks/GotoTask.hpp"
#include "Tasks/OrderedTask.hpp"
#include "Tasks/OnlineContest.hpp"
#include "TaskStats/TaskStats.hpp"
#include "TaskStats/CommonStats.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "TaskEvents.hpp"
#include "TaskBehaviour.hpp"
#include "TaskAdvance.hpp"
#include "Trace/Trace.hpp"
#include "Factory/AbstractTaskFactory.hpp"
#include "Util/Serialisable.hpp"

class Waypoints;
class TaskVisitor;
/**
 *  Main interface exposed to clients for providing access to common types
 *  of navigation tasks.  Hides details of these AbstractTasks behind a facade.
 */
class TaskManager: 
  public TaskInterface,
  public Serialisable,
  private NonCopyable
{
public:
  friend class Serialiser;

  /**
   * Constructor for task manager
   *
   * @param te Task events callback object
   * @param tb Task behaviour options
   * @param wps Waypoint system for use by AbortTask
   *
   * @return Initialised object
   */
  TaskManager(TaskEvents &te,
              const TaskBehaviour &tb,
              const Waypoints &wps);

  ~TaskManager();

  /**
   * Increments active taskpoint sequence for active task
   *
   * @param offset Offset value
   */
  void incrementActiveTaskPoint(int offset);

  /**
   * Sets active taskpoint sequence for active task
   *
   * @param index Sequence number of task point
   */
  void setActiveTaskPoint(unsigned index);

  /**
   * Accessor for active taskpoint sequence for active task
   *
   * @return Sequence number of task point
   */
  gcc_pure
  unsigned getActiveTaskPointIndex() const;

  /**
   * Accessor of current task point of active task
   *
   * @return TaskPoint of active task point, and 0 if no active task
   */
  gcc_pure
  TaskPoint* getActiveTaskPoint() const;

  /**
   * Determine whether active task point optionally shifted points to
   * a valid task point.
   *
   * @param index_offset offset (default 0)
   */
  gcc_pure
  bool validTaskPoint(const int index_offset = 0) const;

  /**
   * Get a random point in the task OZ (for testing simulation route)
   *
   * @param index Index sequence of task point
   * @param mag proportional magnitude of error from center (0-1)
   *
   * @return Location of point
   */
  GEOPOINT random_point_in_task(const unsigned index,
                                const fixed mag=fixed_one) const;

  /**
   * Enumeration of task modes
   */
  enum TaskMode_t {
    MODE_NULL = 0,
    MODE_ORDERED,
    MODE_ABORT,
    MODE_GOTO
  };

  /** Reset the tasks (as if never flown) */
  void reset();

  /** Set active task to abort mode. */
  void abort();

  /**
   * Sets active task to ordered task (or goto if none exists) after
   * goto or aborting.
   */
  void resume();

  /**
   * Sets active task to go to mode, to specified waypoint
   *
   * @param wp Waypoint to go to
   * @return True if successful
   */
  bool do_goto(const Waypoint & wp);

  /**
   * Create a default task if no task is available
   *
   * @param loc Location of aircraft at start
   * @param force Force creation of default task even if a task is present
   */
  void default_task(const GEOPOINT loc, const bool force=false);

  /**
   * Updates internal state of task given new aircraft.
   * Only essential calculations are performed here;
   * other calculations and housekeeping may be performed
   * by update_idle
   *
   * @param state_now Current aircraft state
   * @param state_last Aircraft state at last update
   * @return True if internal state changed
   */
  bool update(const AIRCRAFT_STATE &state_now, 
              const AIRCRAFT_STATE &state_last);

  /**
   * Updates internal state of task to produce
   * auxiliary information or to perform slow house-keeping
   * functions that are non-essential.
   *
   * @param state Current aircraft state
   *
   * @return True if internal state changed
   */
  bool update_idle(const AIRCRAFT_STATE &state);

  /** 
   * Update auto MC.  Internally uses TaskBehaviour to determine settings
   * 
   * @param state_now Current state
   * @param fallback_mc MC value (m/s) to use if algorithm fails or not active
   * 
   * @return True if MC updated
   */
  bool update_auto_mc(const AIRCRAFT_STATE& state_now,
                      const fixed fallback_mc);

  /**
   * Accessor for statistics of active task
   *
   * @return Statistics of active task
   */
  gcc_pure
  const TaskStats& get_stats() const;

  /**
   * Accessor for common statistics
   *
   * @return Statistics
   */
  gcc_pure
  const CommonStats& get_common_stats() const;

  /**
   * Convenience function, determines whether stats are valid
   *
   * @return True if stats valid
   */
  gcc_pure
  bool stats_valid() const;

  /**
   * Size of active task
   *
   * @return Number of taskpoints in active task
   */
  gcc_pure
  unsigned task_size() const;

  /**
   * Check whether ordered task is valid
   *
   * @return True if task is valid
   */
  gcc_pure
  bool check_ordered_task() const;

  /**
   * Check whether active task is valid
   *
   * @return True if task is valid
   */
  gcc_pure
  bool check_task() const;

  /**
   * Accessor for factory system for constructing tasks
   *
   * @return Factory
   */
  gcc_pure
  AbstractTaskFactory& get_factory() const;

  /**
   * Set type of task factory to be used for constructing tasks
   *
   * @param _factory Type of task
   *
   * @return Type of task
   */
  OrderedTask::Factory_t set_factory(const OrderedTask::Factory_t _factory);

  /**
   * Create a clone of the task. 
   * Caller is responsible for destruction.
   *
   * @param te Task events
   * @param tb Task behaviour
   * @param ta Task advance
   * @param gp Glide Polar
   *
   * @return Initialised object
   */
  OrderedTask* clone(TaskEvents &te, 
                     const TaskBehaviour &tb,
                     TaskAdvance &ta,
                     GlidePolar &gp) const;

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @return True if this task changed
   */
  bool commit(const OrderedTask& that);

  /**
   * Accessor for task advance system
   *
   * @return Task advance mechanism
   */
  TaskAdvance& get_task_advance();

  /**
   * Access active task mode
   *
   * @return Active task mode
   */
  gcc_pure
  TaskMode_t get_mode() const;

  /**
   * Determine if the active mode is a particular mode
   *
   * @param the_mode Mode to compare against
   *
   * @return True if modes match
   */
  gcc_pure
  bool is_mode(const TaskMode_t the_mode) const;

  /**
   * Retrieve copy of glide polar used by task system
   *
   * @return Copy of glide polar
   */
  GlidePolar get_glide_polar();

  /**
   * Retrieves glide polar used by task system
   *
   * @return Reference to glide polar
   */
  gcc_pure
  const GlidePolar& get_glide_polar() const;

  /**
   * Update glide polar used by task system
   *
   * @param glide_polar The polar to set to
   */
  void set_glide_polar(const GlidePolar& glide_polar);

  /**
   * Retrieve copy of safety glide polar used by task system
   *
   * @return Copy of glide polar
   */
  gcc_pure
  GlidePolar get_safety_polar() const;

  /**
   * Update safety polar used by task system
   *
   * @param glide_polar The polar to set to
   */
  void set_safety_polar(const GlidePolar& glide_polar);

  /**
   * Retrieve trace vector
   *
   */
  TracePointVector find_trace_points(const GEOPOINT &loc, const fixed range,
                                     const unsigned mintime, const fixed resolution) const;

  /**
   * Retrieve olc solution vector
   *
   * @return Vector of trace points selected for OLC
   */
  gcc_pure
  const TracePointVector& get_olc_points() const;

  /**
   * Retrieve olc trace vector
   *
   * @return Vector of trace points reduced for OLC
   */
  gcc_pure
  const TracePointVector& get_trace_points() const;

  /**
   * Accesses ordered task start state
   *
   * @return State at task start (or null state if not started)
   */
  AIRCRAFT_STATE get_start_state() const;

  /**
   * Accesses ordered task finish state
   *
   * @return State at task finish (or null state if not finished)
   */
  AIRCRAFT_STATE get_finish_state() const;

  /**
   * Return required arrival height of final point in task
   */
  fixed get_finish_height() const;

  /** 
   * Find location of center of task (for rendering purposes)
   * 
   * @param fallback_location Location to use if no task active
   * 
   * @return Location of center of task
   */
  GEOPOINT get_task_center(const GEOPOINT& fallback_location) const;

  /** 
   * Find approximate radius of task from center to edge (for rendering purposes)
   * 
   * @param fallback_location Location to use if no task active
   * 
   * @return Radius (m) from center to edge of task
   */
  fixed get_task_radius(const GEOPOINT& fallback_location) const;

#ifdef DO_PRINT
  void print(const AIRCRAFT_STATE &location);
#endif

  const OrderedTaskBehaviour& get_ordered_task_behaviour() const;

private:
  GlidePolar m_glide_polar;

  Trace trace_full;
  Trace trace_sprint;

  OrderedTask task_ordered;

  GotoTask task_goto;
  
  AbortTask task_abort;

  OnlineContest task_olc;

  const TaskBehaviour &task_behaviour;

  TaskMode_t mode;
  AbstractTask* active_task;
    
  const TaskStats null_stats;

  TaskMode_t set_mode(const TaskMode_t mode);

  TaskAdvance task_advance;

  CommonStats common_stats;

  void update_common_stats(const AIRCRAFT_STATE &state);
  void update_common_stats_times(const AIRCRAFT_STATE &state);
  void update_common_stats_task(const AIRCRAFT_STATE &state);
  void update_common_stats_waypoints(const AIRCRAFT_STATE &state);
  void update_common_stats_polar(const AIRCRAFT_STATE &state);
  
public:
  /**
   * Allow a visitor to visit the active task
   *
   * @param visitor Visitor to accept into the active task
   */
  void CAccept(BaseVisitor& visitor) const;
  void Accept(BaseVisitor& visitor);

  /**
   * Allow a visitor to visit the ordered task
   *
   * @param visitor Visitor to accept into the ordered task
   */
  void ordered_CAccept(BaseVisitor& visitor) const;
  void ordered_Accept(BaseVisitor& visitor);
};
#endif //TASKMANAGER_H
