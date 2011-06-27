/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#ifndef TASKEVENTSPRINT_HPP
#define TASKEVENTSPRINT_HPP

#include "Task/TaskEvents.hpp"

/**
 * TaskEvents to produce simple text output of events for testing
 */
class TaskEventsPrint:
  public TaskEvents
{
public:
  TaskEventsPrint(const bool _verbose): 
    TaskEvents(),
    verbose(_verbose) {};

  void transition_enter(const TaskWaypoint& tp);

  void transition_exit(const TaskWaypoint &tp);

  void transition_alternate();

  void active_advanced(const TaskWaypoint &tp, const int i);

  void active_changed(const TaskWaypoint &tp);

  void warning_start_speed();
  
  void construction_error(const char* error);

  void request_arm(const TaskWaypoint &tp);

  void task_start();

  void task_finish();

  void transition_flight_mode(const bool is_final);

  bool verbose; /**< Option to enable basic output on events (for testing) */
};

#endif
