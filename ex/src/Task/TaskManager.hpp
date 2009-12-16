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

#include "Util/Serialisable.hpp"
#include "Util/NonCopyable.hpp"
#include "Tasks/TaskInterface.hpp"
#include "Tasks/AbortTask.hpp"
#include "Tasks/GotoTask.hpp"
#include "Tasks/OrderedTask.hpp"
#include "TaskStats/TaskStats.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "TaskEvents.hpp"
#include "TaskBehaviour.hpp"

#include "Factory/FAITaskFactory.hpp"
#include "Factory/AATTaskFactory.hpp"
#include "Factory/MixedTaskFactory.hpp"

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
/** 
 * Constructor for task manager
 * 
 * @param te Task events callback object
 * @param tb Task behaviour options
 * @param gp Glide polar used for task calculations
 * @param wps Waypoint system for use by AbortTask
 * 
 * @return Initialised object
 */
  TaskManager(const TaskEvents &te,
              const TaskBehaviour &tb,
              GlidePolar &gp,
              const Waypoints &wps);

/** 
 * Sets active taskpoint sequence for active task
 * 
 * @param index Sequence number of task point
 */
    virtual void setActiveTaskPoint(unsigned index);

/** 
 * Accessor for active taskpoint sequence for active task
 * 
 * @return Sequence number of task point
 */
  virtual unsigned getActiveTaskPointIndex() const;

/** 
 * Accessor of current task point of active task
 * 
 * @return TaskPoint of active task point, and 0 if no active task
 */
  virtual TaskPoint* getActiveTaskPoint() const;

/**
 * Determine whether active task point optionally shifted points to
 * a valid task point.
 *
 * @param index_offset offset (default 0)
 */
  virtual bool validTaskPoint(const int index_offset=0) const;

/** 
 * Get a random point in the task OZ (for testing simulation route)
 * 
 * @param index Index sequence of task point
 * @param mag proportional magnitude of error from center (0-1)
 * 
 * @return Location of point
 */
  GEOPOINT random_point_in_task(const unsigned index, const double mag=1.0) const;

  /**
   * Enumeration of task modes
   */
  enum TaskMode_t {
    MODE_NULL=0,
    MODE_ORDERED,
    MODE_ABORT,
    MODE_GOTO
  };

  /**
   * Enumeration of factory types.  This is the set of
   * types of ordered task that can be created.
   */
  enum Factory_t {
    FACTORY_FAI=0,
    FACTORY_AAT,
    FACTORY_MIXED
  };

  /** 
   * Reset the tasks (as if never flown)
   * 
   */
  void reset();

/** 
 * Set active task to abort mode.
 * 
 */
  void abort();

/** 
 * Sets active task to ordered task (or goto if none exists) after
 * goto or aborting.
 * 
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
 * Updates internal state of task given new aircraft.
 * Only essential calculations are performed here;
 * other calculations and housekeeping may be performed
 * by update_idle
 * 
 * @param state_now Current aircraft state
 * @param state_last Aircraft state at last update 
 * @return True if internal state changed
 */
  virtual bool update(const AIRCRAFT_STATE &state_now, 
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
  virtual bool update_idle(const AIRCRAFT_STATE &state);

/** 
 * Accessor for statistics of active task
 * 
 * @return Statistics of active task
 */
  virtual const TaskStats& get_stats() const;

/** 
 * Convenience function, determines whether stats are valid
 * 
 * @return True if stats valid
 */
  bool stats_valid() const {
    return get_stats().task_valid;
  }

/** 
 * Size of active task
 * 
 * @return Number of taskpoints in active task
 */
  unsigned task_size() const;

/** 
 * Check whether ordered task is valid
 * 
 * @return True if task is valid
 */
  bool check_ordered_task() const;

/** 
 * Check whether active task is valid
 * 
 * @return True if task is valid
 */
  bool check_task() const;

#ifdef DO_PRINT
  virtual void print(const AIRCRAFT_STATE &location);
#endif

/** 
 * Accessor for factory system for constructing tasks
 * 
 * @return Factory
 */
  AbstractTaskFactory* get_factory() const {
    return active_factory;
  }

/** 
 * Set type of task factory to be used for constructing tasks
 * 
 * @param _factory Type of task
 * 
 * @return Type of task
 */
  Factory_t set_factory(const Factory_t _factory);

/** 
 * Accessor for task advance system
 * 
 * @return Task advance mechanism
 */
  TaskAdvance& get_task_advance() {
    return task_advance;
  }

/** 
 * Access active task mode
 * 
 * @return Active task mode
 */
  TaskMode_t get_mode() const {
    return mode;
  }

/** 
 * Determine if the active mode is a particular mode
 * 
 * @param the_mode Mode to compare against
 * 
 * @return True if modes match
 */
  bool is_mode(const TaskMode_t the_mode) const {
    return mode == the_mode;
  }

private:
  /** @link aggregation */
  OrderedTask task_ordered;

  /** @link aggregation */
  GotoTask task_goto;
  
  /** @link aggregation */
  AbortTask task_abort;

  const TaskBehaviour &task_behaviour;

  /** @link aggregation */
  FAITaskFactory factory_fai;

  /** @link aggregation */
  AATTaskFactory factory_aat;

  /** @link aggregation */
  MixedTaskFactory factory_mixed;

  TaskMode_t mode;
  AbstractTask* active_task;
  Factory_t factory_mode;
  AbstractTaskFactory* active_factory;
    
  const TaskStats null_stats;

  TaskMode_t set_mode(const TaskMode_t mode);

  /** @link aggregation */
  TaskAdvance task_advance;
  
public:
  /**
   * Allow a visitor to visit the active task
   *
   * @param visitor Visitor to accept into the active task
   */
  void Accept(BaseVisitor& visitor) const;

  /**
   * Allow a visitor to visit the ordered task
   *
   * @param visitor Visitor to accept into the ordered task
   */
  void ordered_Accept(BaseVisitor& visitor) const;
};
#endif //TASKMANAGER_H
