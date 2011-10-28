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

#include "Task/TaskStore.hpp"
#include "Task/TaskFile.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Components.hpp"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"
#include "LocalPath.hpp"
#include "Language/Language.hpp"

#include <cstdio>
#include <algorithm>

class TaskFileVisitor: public File::Visitor
{
private:
  TaskStore::ItemVector &store;

public:
  TaskFileVisitor(TaskStore::ItemVector &_store):
    store(_store) {}

  void Visit(const TCHAR *path, const TCHAR *filename) {
    // Create a TaskFile instance to determine how many
    // tasks are inside of this task file
    TaskFile* task_file = TaskFile::Create(path);
    if (task_file == NULL)
      return;

    // Get base name of the task file
    const TCHAR* base_name = BaseName(path);

    // Count the tasks in the task file
    unsigned count = task_file->Count();
    // For each task in the task file
    for (unsigned i = 0; i < count; i++) {
      // Copy base name of the file into task name
      TCHAR name[255];
      _tcscpy(name, (base_name != NULL) ? base_name : path);

      // If the task file holds more than one task
      if (count > 1) {
        if (i < task_file->namesuffixes.size() &&
            task_file->namesuffixes[i]) {

          _tcscat(name, _T(": "));
          _tcscat(name, task_file->namesuffixes[i]);

        } else {
          // .. append " - Task #[n]" suffix to the task name
          TCHAR suffix[255];
          _stprintf(suffix, _T(": %s #%2d"), _("Task"), i + 1);
          _tcscat(name, suffix);
        }
      }

      // Add the task to the TaskStore
      store.push_back(TaskStore::Item(path, (name == NULL) ? path : name, i));
    }

    // Remove temporary TaskFile instance
    delete task_file;
  }
};

void
TaskStore::Clear()
{
  // clear entries first
  store.erase(store.begin(), store.end());
}

void
TaskStore::Scan()
{
  Clear();

  // scan files
  TaskFileVisitor tfv(store);
  VisitDataFiles(_T("*.tsk"), tfv);
  VisitDataFiles(_T("*.cup"), tfv);

  std::sort(store.begin(), store.end());
}

size_t
TaskStore::Size() const
{
  return store.size();
}

TaskStore::Item::Item(const tstring &_filename, const tstring _task_name,
                      unsigned _task_index):
  task_name(_task_name),
  filename(_filename),
  task_index(_task_index),
  task(NULL),
  valid(true)
{        
}

TaskStore::Item::~Item()
{
  if (!filename.empty())
    delete task;
}

OrderedTask*
TaskStore::Item::GetTask()
{
  if (task != NULL)
    return task;

  if (valid)
    task = protected_task_manager->
      TaskCreate(filename.c_str(), &way_points, task_index);

  if (task == NULL)
    valid = false;

  return task;
}

const TCHAR *
TaskStore::Item::GetName() const
{
  return task_name.c_str();
}

bool
TaskStore::Item::operator<(const Item &other) const
{
  return _tcscmp(GetName(), other.GetName()) == -1;
}

const TCHAR *
TaskStore::GetName(unsigned index) const
{
  return store[index].GetName();
}

OrderedTask* 
TaskStore::GetTask(unsigned index)
{
  return store[index].GetTask();
}
