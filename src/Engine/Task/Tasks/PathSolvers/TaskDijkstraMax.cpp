/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "TaskDijkstraMax.hpp"
#include "Task/Tasks/OrderedTask.hpp"

TaskDijkstraMax::TaskDijkstraMax(OrderedTask& _task) :
  TaskDijkstra(_task)
{
}

bool
TaskDijkstraMax::distance_max()
{
  if (!refresh_task())
    return false;

  const ScanTaskPoint start(0, 0);
  
  // dont reserve queue size because this is temporary
  DijkstraTaskPoint dijkstra(start, false, 0);

  const bool retval = distance_general(dijkstra);
  if (retval)
    save();

  return retval;
}


void
TaskDijkstraMax::save()
{
  for (unsigned j = 0; j != num_stages; ++j) {
    task.set_tp_search_max(j, solution[j]);
    if (j <= active_stage)
      task.set_tp_search_achieved(j, solution[j]);
  }
}
