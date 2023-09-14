// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "util/tstring.hpp"

#include <memory>
#include <vector>

struct TaskBehaviour;
class OrderedTask;
class Waypoints;

/**
 * Class to load multiple tasks on demand, e.g. for browsing
 */
class TaskStore 
{
public:
  struct Item
  {
    tstring task_name;
    AllocatedPath filename;
    unsigned task_index;
    std::unique_ptr<OrderedTask> task;
    bool valid;

    Item(Path the_filename,
         tstring::const_pointer _task_name,
         unsigned _task_index = 0)
      :task_name(_task_name),
       filename(the_filename),
       task_index(_task_index),
       valid(true) {}

    ~Item() noexcept;

    Item(Item &&) = default;
    Item &operator=(Item &&) = default;

    [[gnu::pure]]
    tstring::const_pointer GetName() const {
      return task_name.c_str();
    }

    [[gnu::pure]]
    Path GetPath() const {
      return filename;
    }

    const OrderedTask *GetTask(const TaskBehaviour &task_behaviour,
                               Waypoints *waypoints) noexcept;

    [[gnu::pure]]
    bool operator<(const TaskStore::Item &other) const {
      return task_name.compare(other.task_name) < 0;
    }
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
   *
   * @param extra scan all "extra" (non-XCSoar) task files, e.g. *.cup
   * and task declarations from *.igc
   */
  void Scan(bool extra=false);

  /**
   * Clear all the tasks from the TaskStore
   */
  void Clear();

  /**
   * Return the number of tasks in the TaskStore
   * @return The number of tasks in the TaskStore
   */
  [[gnu::pure]]
  size_t Size() const {
    return store.size();
  }

  /**
   * Return the filename of the task defined by the given index
   * (e.g. TestTask.tsk)
   * @param index TaskStore index of the desired Task
   * @return Filename of the task defined by the given index
   */
  [[gnu::pure]]
  tstring::const_pointer GetName(unsigned index) const;

  /**
   * Return the pathname of the task defined by the given index
   * (e.g. tasks/TestTask.tsk)
   * @param index TaskStore index of the desired Task
   * @return pathname of the task defined by the given index
   */
  [[gnu::pure]]
  Path GetPath(unsigned index) const;

  /**
   * Return the task defined by the given index
   * @param index TaskStore index of the desired Task
   * @return The task defined by the given index
   */
  const OrderedTask *GetTask(unsigned index,
                             const TaskBehaviour &task_behaviour,
                             Waypoints *waypoints);
};
