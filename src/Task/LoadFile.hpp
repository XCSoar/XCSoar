/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef TASK_LOAD_FILE_HPP
#define TASK_LOAD_FILE_HPP

#include <memory>

class Path;
class OrderedTask;
class Waypoints;
struct TaskBehaviour;

/**
 * Throws on error.
 */
std::unique_ptr<OrderedTask>
LoadTask(Path path, const TaskBehaviour &task_behaviour,
         const Waypoints *waypoints=nullptr);

#endif
