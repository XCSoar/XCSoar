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
#include "Task/ProtectedTaskManager.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Components.hpp"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"
#include "LocalPath.hpp"

class TaskFileVisitor: public File::Visitor
{
private:
  TaskStore::ItemVector &m_store;

public:
  TaskFileVisitor(TaskStore::ItemVector &store):
    m_store(store) {}

  void Visit(const TCHAR* path, const TCHAR* filename) {
    m_store.push_back(TaskStore::Item(path));
  }
};

void
TaskStore::clear()
{
  // clear entries first
  m_store.erase(m_store.begin(), m_store.end());
}

void
TaskStore::scan()
{
  clear();

  // scan files
  TaskFileVisitor tfv(m_store);
  const TCHAR* data_path = GetPrimaryDataPath();
  Directory::VisitSpecificFiles(data_path, _T("*.tsk"), tfv, true);

  sort(m_store.begin(), m_store.end());
}

size_t
TaskStore::size() const
{
  return m_store.size();
}

TaskStore::Item::Item(const tstring &the_filename):
  filename(the_filename),
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
TaskStore::Item::get_task()
{
  if (task != NULL)
    return task;

  if (valid)
    task = protected_task_manager->task_create(filename.c_str(), &way_points);

  if (task == NULL)
    valid = false;

  return task;
}

const TCHAR *
TaskStore::Item::get_name() const
{
  const TCHAR *path = filename.c_str();
  const TCHAR *name = BaseName(path);

  return (name == NULL) ? path : name;
}

bool
TaskStore::Item::operator<(const Item &i2) const
{
  return _tcscmp(get_name(), i2.get_name()) == -1;
}

const TCHAR *
TaskStore::get_name(unsigned index) const
{
  return m_store[index].get_name();
}

OrderedTask* 
TaskStore::get_task(unsigned index)
{
  return m_store[index].get_task();
}
