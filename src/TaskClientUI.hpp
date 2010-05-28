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
#ifndef TASKCLIENTUI_HPP
#define TASKCLIENTUI_HPP

#include <tchar.h>
#include "TaskClientMap.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Task/TaskAdvance.hpp"
#include "Waypoint/WaypointSorter.hpp"

class Declaration;
class Waypoint;
class RasterTerrain;
class SETTINGS_COMPUTER;

/** Facade class for protected access to task data by GUI/user threads */
class TaskClientUI: public TaskClientMap
{
public:
  TaskClientUI(TaskManager& tm,
               const TaskBehaviour& tb,
               TaskEvents& te,
               Waypoints& waypoints):
    TaskClientMap(tm, waypoints),
    task_behaviour(tb),
    task_events(te),
    glide_polar(tm.get_glide_polar()) {};

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
  bool waypoint_is_writable(const Waypoint& wp) const;
  bool check_duplicate_waypoints(OrderedTask& ordered_task);
  void set_waypoint_details(const Waypoint& wp, const tstring& Details);
  WaypointSelectInfoVector get_airports(const GEOPOINT &loc) const;
  Waypoint create_waypoint(const GEOPOINT &location);
  void append_waypoint(Waypoint& wp);
  void replace_waypoint(const Waypoint& wp, Waypoint& copy);
  void optimise_waypoints();
  void set_home(const RasterTerrain &terrain,
                SETTINGS_COMPUTER &settings,
                const bool reset, const bool set_location= false);
  const Waypoint* get_nearest_waypoint(const GEOPOINT& location) const;

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

protected:
  const TaskBehaviour &task_behaviour;
  TaskEvents &task_events;
  GlidePolar glide_polar;

  static const TCHAR default_task_path[];
};


#endif
