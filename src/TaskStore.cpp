/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "TaskStore.hpp"
#include "DataField/FileReader.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/Tasks/OrderedTask.hpp"
#include "Components.hpp"
#include "OS/PathName.hpp"

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
  DataFieldFileReader fr(NULL);
  fr.ScanDirectoryTop(_T("*.tsk"));
  fr.Sort();

  // append to list
  for (unsigned i = 1; i < fr.size(); i++) {
    m_store.push_back(TaskStoreItem(fr.getItem(i)));
  }
}

size_t
TaskStore::size() const
{
  return m_store.size();
}

TaskStore::TaskStoreItem::TaskStoreItem():
  filename(_T("unk")),
  task(NULL),
  valid(false)
{
}

TaskStore::TaskStoreItem::TaskStoreItem(const tstring &the_filename):
  filename(the_filename),
  task(NULL),
  valid(true)
{        
}

TaskStore::TaskStoreItem::~TaskStoreItem() 
{
  if (!filename.empty())
    delete task;
}

OrderedTask*
TaskStore::TaskStoreItem::get_task() 
{
  if (task != NULL)
    return task;

  if (valid)
    task = protected_task_manager.task_create(filename.c_str());

  if (task == NULL)
    valid = false;

  return task;
}

const TCHAR *
TaskStore::get_name(unsigned index) const
{
  const TCHAR *path = m_store[index].filename.c_str();
  const TCHAR *name = BaseName(path);
  if (name == NULL)
    name = path;
  return name;
}

OrderedTask* 
TaskStore::get_task(unsigned index)
{
  return m_store[index].get_task();
}
