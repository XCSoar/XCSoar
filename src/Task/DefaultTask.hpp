/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef DEFAULT_TASK_HPP
#define DEFAULT_TASK_HPP

#include "Compiler.h"

struct TaskBehaviour;
class OrderedTask;
class Waypoints;

#define default_task_path _T("Default.tsk")

/**
 * Creates an ordered task based on the Default.tsk file
 * Consumer's responsibility to delete task
 *
 * @param waypoints waypoint structure
 * @param failfactory default task type used if Default.tsk is invalid
 * @return OrderedTask from Default.tsk file or if Default.tsk is invalid
 * or non-existent, returns empty task with defaults set by
 * config task defaults
 */
gcc_malloc
OrderedTask *
LoadDefaultTask(const TaskBehaviour &task_behaviour,
                const Waypoints *waypoints);

#endif
