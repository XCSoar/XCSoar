/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "TaskDijkstra.hpp"
#include "Task/Tasks/OrderedTask.hpp"

TaskDijkstra::TaskDijkstra(OrderedTask& _task, bool is_min):
  NavDijkstra(is_min, 0),
  task(_task)
{
}

bool
TaskDijkstra::refresh_task()
{
  set_stages(task.TaskSize());
  if (num_stages < 2)
    return false;

  active_stage = task.GetActiveTaskPointIndex();

  for (unsigned stage = 0; stage != num_stages; ++stage)
    sp_sizes[stage] = task.get_tp_search_points(stage).size();

  return true;
}

const SearchPoint &
TaskDijkstra::GetPointFast(const ScanTaskPoint sp) const
{
  return task.get_tp_search_points(sp.GetStageNumber())[sp.GetPointIndex()];
}

void
TaskDijkstra::add_edges(const ScanTaskPoint curNode)
{
  ScanTaskPoint destination(curNode.GetStageNumber() + 1, 0);
  const unsigned dsize = get_size(destination.GetStageNumber());

  for (const ScanTaskPoint end(destination.GetStageNumber(), dsize);
       destination != end; destination.IncrementPointIndex())
    dijkstra.link(destination, curNode, distance(curNode, destination));
}

void 
TaskDijkstra::add_start_edges(const SearchPoint &currentLocation)
{
  ScanTaskPoint destination(active_stage, 0);
  const unsigned dsize = get_size(active_stage);

  for (const ScanTaskPoint end(active_stage, dsize);
       destination != end; destination.IncrementPointIndex())
    dijkstra.link(destination, destination,
                  distance(destination, currentLocation));
}

bool
TaskDijkstra::run()
{
  solution_valid = false;
  const bool retval = distance_general() && solution_valid;
  dijkstra.clear();
  return retval;
}
