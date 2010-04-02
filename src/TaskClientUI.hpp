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
#include "TaskClient.hpp"
#include "Task/TaskAdvance.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Task/TaskManager.hpp"

class TaskClientUI: public TaskClient
{
public:
  TaskClientUI(TaskManager& tm,
               const TaskBehaviour& tb,
               TaskEvents& te):
    TaskClient(tm),
    task_behaviour(tb),
    task_events(te),
    glide_polar(tm.get_glide_polar()) {};

  TaskAdvance::TaskAdvanceMode_t get_advance_mode() const;
  void set_advance_mode(TaskAdvance::TaskAdvanceMode_t the_mode);
  void set_advance_armed(const bool do_armed);
  bool is_advance_armed() const;
  bool toggle_advance_armed();

  GlidePolar get_safety_polar() const;

  const Waypoint* getActiveWaypoint() const;

  void incrementActiveTaskPoint(int offset);

  TaskManager::TaskMode_t get_mode() const;
  bool do_goto(const Waypoint & wp);
  void abort();
  void resume();

  AIRCRAFT_STATE get_start_state() const;
  fixed get_finish_height() const;

  const TracePointVector get_trace_points();
  const TracePointVector get_olc_points();

  bool check_ordered_task() const;
  bool check_task() const;

  GEOPOINT get_task_center(const GEOPOINT& fallback_location) const;
  fixed get_task_radius(const GEOPOINT& fallback_location) const;

  void CAccept(BaseVisitor& visitor) const;
  void ordered_CAccept(BaseVisitor& visitor) const;

  TracePointVector find_trace_points(const GEOPOINT &loc, 
                                     const fixed range,
                                     const unsigned mintime, 
                                     const fixed resolution) const;

  // 
  OrderedTask* task_clone();

  /**
   * Copy task into this task
   *
   * @param other OrderedTask to copy
   * @return True if this task changed
   */
  bool task_commit(const OrderedTask& that);

  const OrderedTaskBehaviour get_ordered_task_behaviour() const;

  bool task_save(const TCHAR* path);
  bool task_load(const TCHAR* path);
  bool task_save_default();
  bool task_load_default();

protected:
  const TaskBehaviour &task_behaviour;
  TaskEvents &task_events;
  GlidePolar glide_polar;
  TaskAdvance task_advance;

  static const TCHAR default_task_path[];
};


#endif
