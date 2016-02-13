/*
Copyright_License {

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

#include "Task/TaskFile.hpp"
#include "Task/TaskFileXCSoar.hpp"
#include "Task/TaskFileSeeYou.hpp"
#include "Task/TaskFileIGC.hpp"

#include <stdlib.h>
#include <memory>

TaskFile::~TaskFile()
{
  for (unsigned i = 0; i < namesuffixes.size(); i++)
    free ((TCHAR*)namesuffixes[i]);
}
TaskFile*
TaskFile::Create(Path path)
{
  // If XCSoar task file -> return new TaskFileXCSoar
  if (path.MatchesExtension(_T(".tsk")))
    return new TaskFileXCSoar(path);

  // If SeeYou task file -> return new TaskFileSeeYou
  if (path.MatchesExtension(_T(".cup")))
    return new TaskFileSeeYou(path);

  // If IGC file -> return new TaskFileIGC
  if (path.MatchesExtension(_T(".igc")))
    return new TaskFileIGC(path);

  // unknown task file type
  return nullptr;
}

OrderedTask *
TaskFile::GetTask(Path path, const TaskBehaviour &task_behaviour,
                  const Waypoints *waypoints, unsigned index)
{
  std::unique_ptr<TaskFile> file(TaskFile::Create(path));
  if (!file)
    return nullptr;

  return file->GetTask(task_behaviour, waypoints, index);
}

const TCHAR *
TaskFile::GetName(unsigned index) const
{
  if (index >= namesuffixes.size())
    return nullptr;

  return namesuffixes[index];
}
