/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

struct Waypoint;
class OrderedTask;

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
  enum TaskEditResult {
    SUCCESS,
    UNMODIFIED,
    INVALID,
    NOTASK,
    MUTATED_TO_GOTO,
    MUTATED_FROM_GOTO,
  };

  TaskEditResult AppendToTask(const Waypoint &wp);
  TaskEditResult InsertInTask(const Waypoint &wp);

  TaskEditResult ReplaceInTask(const Waypoint &wp);
  TaskEditResult RemoveFromTask(const Waypoint &wp);

  /**
   * @param wp
   * @return TurnPointIndex if MODE_ORDERED and wp is in task
   * else returns -1
   */
  int GetIndexInTask(const Waypoint &wp);

  /**
   * is this wp an unachieved task point?
   * @param waypoint.  the wp being tested
   * @return.  -1 or last index in ordered task where the wp is an
   * unachieved intermediate point.  -1 if mode != MODE_ORDDERED
   */
  int GetUnachievedIndexInTask(const Waypoint &waypoint);
};

#endif
