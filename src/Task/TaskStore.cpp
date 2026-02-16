// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task/TaskStore.hpp"
#include "Task/TaskFile.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "LocalPath.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"

#include <algorithm>
#include <memory>

class TaskFileVisitor: public File::Visitor
{
private:
  TaskStore::ItemVector &store;

public:
  TaskFileVisitor(TaskStore::ItemVector &_store):
    store(_store) {}

  void Visit(Path path, Path base_name) override
  try {
    // Create a TaskFile instance to determine how many
    // tasks are inside of this task file
    const auto task_file = TaskFile::Create(path);
    if (!task_file)
      return;

    const auto list = task_file->GetList();

    // Count the tasks in the task file
    unsigned count = list.size();
    // For each task in the task file
    for (unsigned i = 0; i < count; i++) {
      // Copy base name of the file into task name
      StaticString<256> name(base_name.c_str());

      // If the task file holds more than one task
      const auto &saved_name = list[i];
      if (!saved_name.empty()) {
        name += ": ";
        name += saved_name.c_str();
      } else if (count > 1) {
        // .. append " - Task #[n]" suffix to the task name
        name.AppendFormat(": %s #%d", _("Task"), i + 1);
      }

      // Add the task to the TaskStore
      store.emplace_back(path, name.empty() ? path.c_str() : name, i);
    }
  } catch (...) {
    LogError(std::current_exception());
  }
};

void
TaskStore::Clear()
{
  // clear entries first
  store.erase(store.begin(), store.end());
}

void
TaskStore::Scan(bool extra)
{
  Clear();

  // scan files
  TaskFileVisitor tfv(store);
  VisitDataFiles("*.tsk", tfv);

  if (extra) {
    VisitDataFiles("*.cup", tfv);
    VisitDataFiles("*.igc", tfv);
  }

  std::sort(store.begin(), store.end());
}

TaskStore::Item::~Item() noexcept = default;

const OrderedTask *
TaskStore::Item::GetTask(const TaskBehaviour &task_behaviour,
                         Waypoints *waypoints) noexcept
{
  if (task != nullptr)
    return task.get();

  if (valid)
    task = TaskFile::GetTask(filename, task_behaviour,
                             waypoints, task_index);

  if (task == nullptr)
    valid = false;
  else
    task->UpdateGeometry();

  return task.get();
}

const char *
TaskStore::GetName(unsigned index) const
{
  return store[index].GetName();
}

Path
TaskStore::GetPath(unsigned index) const
{
  return store[index].GetPath();
}

const OrderedTask *
TaskStore::GetTask(unsigned index, const TaskBehaviour &task_behaviour,
                   Waypoints *waypoints)
{
  return store[index].GetTask(task_behaviour, waypoints);
}
