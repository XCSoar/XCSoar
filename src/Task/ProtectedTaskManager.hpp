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

#ifndef XCSOAR_PROTECTED_TASK_MANAGER_HPP
#define XCSOAR_PROTECTED_TASK_MANAGER_HPP

#include "Thread/Mutex.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/TaskManager.hpp"
#include "Task/TaskAdvance.hpp"

class TaskStats;
class CommonStats;
class RasterTerrain;

/**
 * Facade to task/airspace/waypoints as used by threads,
 * to manage locking
 */
class ProtectedTaskManager
{
protected:
  TaskManager& task_manager;
  static Mutex mutex;

  const TaskBehaviour &task_behaviour;
  TaskEvents &task_events;
  GlidePolar glide_polar;

  static const TCHAR default_task_path[];

public:
  ProtectedTaskManager(TaskManager &_task_manager, const TaskBehaviour& tb,
                       TaskEvents& te)
    :task_manager(_task_manager),
     task_behaviour(tb), task_events(te),
     glide_polar(_task_manager.get_glide_polar()) {}

// common accessors for ui and calc clients
  GlidePolar get_glide_polar() const;
  void set_glide_polar(const GlidePolar& glide_polar);

  bool check_task() const;
  TaskManager::TaskMode_t get_mode() const;

  // trace points
  TracePointVector find_trace_points(const GEOPOINT &loc, 
                                     const fixed range,
                                     const unsigned mintime, 
                                     const fixed resolution) const;

  void CAccept(TaskVisitor &visitor) const;
  void ordered_CAccept(TaskVisitor &visitor) const;
  const OrderedTaskBehaviour get_ordered_task_behaviour() const;


  TaskAdvance::TaskAdvanceState_t get_advance_state() const;

/*
  TaskAdvance::TaskAdvanceMode_t get_advance_mode() const;
  void set_advance_mode(TaskAdvance::TaskAdvanceMode_t the_mode);
*/
  void set_advance_armed(const bool do_armed);
  bool is_advance_armed() const;
  bool toggle_advance_armed();

  GlidePolar get_safety_polar() const;

  const Waypoint* getActiveWaypoint() const;

  void incrementActiveTaskPoint(int offset);

  bool do_goto(const Waypoint & wp);
  void abort();
  void resume();

  AIRCRAFT_STATE get_start_state() const;
  fixed get_finish_height() const;

  const TracePointVector get_trace_points();
  const TracePointVector get_olc_points();

  bool check_ordered_task() const;

  GEOPOINT get_task_center(const GEOPOINT& fallback_location) const;
  fixed get_task_radius(const GEOPOINT& fallback_location) const;

  // waypoints
  bool read_waypoints(const RasterTerrain *terrain);
  void save_waypoints();
  void close_waypoints();
  bool check_duplicate_waypoints(OrderedTask& ordered_task,
                                 Waypoints &way_points);

  // 
  OrderedTask* task_clone();
  OrderedTask* task_blank();

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @return True if this task changed
   */
  bool task_commit(const OrderedTask& that);

  bool task_save(const TCHAR* path);
  bool task_load(const TCHAR* path);
  bool task_save_default();
  bool task_load_default();
  OrderedTask* task_copy(const OrderedTask& that);
  OrderedTask* task_create(const TCHAR* path);
  bool task_save(const TCHAR* path, const OrderedTask& task);



  /** Reset the tasks (as if never flown) */
  void reset();

  bool update(const AIRCRAFT_STATE &state_now, 
              const AIRCRAFT_STATE &state_last);

  bool update_idle(const AIRCRAFT_STATE &state);

  bool update_auto_mc(const AIRCRAFT_STATE& state_now,
                      const fixed fallback_mc);

  void set_task_behaviour(const TaskBehaviour& behaviour);

  const TaskStats& get_stats() const;
  const CommonStats& get_common_stats() const;


};


#endif
