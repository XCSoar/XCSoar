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

#ifndef XCSOAR_TASK_FILE_HPP
#define XCSOAR_TASK_FILE_HPP

#include "system/Path.hpp"
#include "util/tstring.hpp"

#include <memory>
#include <vector>

#include <tchar.h>

struct TaskBehaviour;
class Waypoints;
class OrderedTask;

class TaskFile
{
protected:
  AllocatedPath path;

public:
  explicit TaskFile(Path _path) noexcept
    :path(_path) {}

  virtual ~TaskFile() noexcept = default;

  /**
   * Creates a TaskFile instance according to the extension
   * @param filename The filepath
   * @return TaskFile instance
   */
  static std::unique_ptr<TaskFile> Create(Path path);

  static std::unique_ptr<OrderedTask> GetTask(Path path,
                                              const TaskBehaviour &task_behaviour,
                                              const Waypoints *waypoints,
                                              unsigned index);

  /**
   * Throws on error.
   */
  virtual std::vector<tstring> GetList() const = 0;

  virtual std::unique_ptr<OrderedTask> GetTask(const TaskBehaviour &task_behaviour,
                                               const Waypoints *waypoints,
                                               unsigned index) const = 0;
};

#endif
