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

#include "Task/TaskFile.hpp"

#include "Task/TaskFileXCSoar.hpp"
#include "Task/TaskFileSeeYou.hpp"

#include "OS/FileUtil.hpp"
#include "UtilsFile.hpp"
#include "Util/StringUtil.hpp"

#include <assert.h>
#include <stdlib.h>

TaskFile::~TaskFile()
{
  for (unsigned i = 0; i < namesuffixes.size(); i++)
    free ((TCHAR*)namesuffixes[i]);
}
TaskFile*
TaskFile::Create(const TCHAR* path)
{
  // If filename is empty or file does not exist
  if (string_is_empty(path) || !File::Exists(path))
    return NULL;

  // If XCSoar task file -> return new XCSoarTaskFile
  if (MatchesExtension(path, _T(".tsk")))
    return new TaskFileXCSoar(path);

  // If XCSoar task file -> return new XCSoarTaskFile
  if (MatchesExtension(path, _T(".cup")))
    return new TaskFileSeeYou(path);

  // unknown task file type
  return NULL;
}

