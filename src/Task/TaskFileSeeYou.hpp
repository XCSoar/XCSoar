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

#ifndef XCSOAR_TASK_FILE_SEEYOU_HPP
#define XCSOAR_TASK_FILE_SEEYOU_HPP

#include "TaskFile.hpp"
#include "OS/Path.hpp"

/**
 * A class that reads and parses a SeeYou task file to an XCSoar internal
 * task representation.
 */

class TaskFileSeeYou: public TaskFile
{
public:
  explicit TaskFileSeeYou(Path _path):TaskFile(_path) {}

  /**
   * Give the task produced by parsing the SeeYou file.
   * @param task_behaviour The type of the task.
   * @param waypoints All waypoints contained in the SeeYou task file.
   * @param index Index into the array of tasks in the SeeYou file, 0....n
   */
  virtual OrderedTask *GetTask(const TaskBehaviour &task_behaviour,
                               const Waypoints *waypoints,
                               unsigned index) const;

  unsigned Count();
};

#endif
