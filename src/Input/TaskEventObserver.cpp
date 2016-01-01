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

#include "TaskEventObserver.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Unordered/AlternateList.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "InputQueue.hpp"

static unsigned
GetBestAlternateID(const TaskManager &tm)
{
  const auto &alternates = tm.GetAlternates();
  return alternates.empty()
    ? unsigned(-1)
    : alternates.front().waypoint->id;
}

void
TaskEventObserver::Check(const TaskManager &tm)
{
  const unsigned new_best_alternate_id = GetBestAlternateID(tm);
  if (new_best_alternate_id != best_alternate_id) {
    best_alternate_id = new_best_alternate_id;
    InputEvents::processGlideComputer(GCE_ALTERNATE_CHANGED);
  }
}
