/*
Copyright_License {

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

#ifndef TASK_STORE_HPP
#define TASK_STORE_HPP

#include "Util/tstring.hpp"
#include <vector>

class OrderedTask;

/**
 * Class to load multiple tasks on demand, e.g. for browsing
 */
class TaskStore 
{
public:
  struct Item
  {
    tstring task_name;
    tstring filename;
    unsigned task_index;
    OrderedTask* task;
    bool valid;

    Item(const tstring &the_filename, const tstring _task_name,
         unsigned _task_index = 0);
    ~Item();

    const TCHAR* GetName() const;
    OrderedTask* GetTask();

    bool operator<(const TaskStore::Item &other) const;
  };

  typedef std::vector<TaskStore::Item> ItemVector;

private:
  /**
   * Internal task storage
   */
  ItemVector store;

public:
  /**
   * Scan the XCSoarData folder for .tsk files and add them to the TaskStore
   */
  void Scan();

  /**
   * Clear all the tasks from the TaskStore
   */
  void Clear();

  /**
   * Return the number of tasks in the TaskStore
   * @return The number of tasks in the TaskStore
   */
  size_t Size() const;

  /**
   * Return the filename of the task defined by the given index
   * (e.g. TestTask.tsk)
   * @param index TaskStore index of the desired Task
   * @return Filename of the task defined by the given index
   */
  const TCHAR *GetName(unsigned index) const;

  /**
   * Return the task defined by the given index
   * @param index TaskStore index of the desired Task
   * @return The task defined by the given index
   */
  OrderedTask* GetTask(unsigned index);
};

#endif
