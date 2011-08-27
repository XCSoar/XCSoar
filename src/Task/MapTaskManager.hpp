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

#ifndef MAPTASKMANAGER_H
#define MAPTASKMANAGER_H

#include "Waypoint/Waypoint.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Task/TaskEvents.hpp"


/**
 * Appends wp to current Ordered task and activates the ordered task if
 * the current ordered task is valid.
 * If the current ordered task is invalid or empty, then
 * either creates a Goto task with the selected waypoint, or if in Goto mode
 * already, it creates an ordered task from the previous Goto point and the
 * selected waypoint.
 */
namespace MapTaskManager
{
  enum task_edit_result {
    SUCCESS,
    UNMODIFIED,
    INVALID,
    NOTASK,
    MUTATED_TO_GOTO,
    MUTATED_FROM_GOTO,
  };

  task_edit_result append_to_task(const Waypoint &wp);
  task_edit_result insert_in_task(const Waypoint &wp);
  task_edit_result replace_in_task(const Waypoint &wp);
  task_edit_result remove_from_task(const Waypoint &wp);

  /**
   * @param wp
   * @return TurnPointIndex if MODE_ORDERED and wp is in task
   * else returns -1
   */
  int index_of_point_in_task(const Waypoint &wp);
};

#endif
