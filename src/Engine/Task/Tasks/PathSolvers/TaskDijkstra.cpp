/* Copyright_License {

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

#include "TaskDijkstra.hpp"
#include "Task/Tasks/OrderedTask.hpp"

TaskDijkstra::TaskDijkstra(OrderedTask& _task, bool is_min, const bool do_reserve):
  NavDijkstra<SearchPoint> (is_min, 0, do_reserve? DIJKSTRA_QUEUE_SIZE: 0),
  task(_task)
{
}

bool
TaskDijkstra::refresh_task()
{
  set_stages(task.task_size());
  if (num_stages < 2)
    return false;

  active_stage = task.getActiveTaskPointIndex();
  calculate_sizes();
  return true;
}

void
TaskDijkstra::calculate_sizes()
{
  for (unsigned stage = 0; stage != num_stages; ++stage)
    sp_sizes[stage] = task.get_tp_search_points(stage).size();
}

unsigned
TaskDijkstra::get_size(const unsigned stage) const
{
  return sp_sizes[stage];
}

const SearchPoint &
TaskDijkstra::GetPointFast(const ScanTaskPoint &sp) const
{
  return task.get_tp_search_points(sp.stage_number)[sp.point_index];
}

const SearchPoint &
TaskDijkstra::get_point(const ScanTaskPoint &sp) const
{
  return task.get_tp_search_points(sp.stage_number)[sp.point_index];
}

void
TaskDijkstra::add_edges(const ScanTaskPoint& curNode)
{
  ScanTaskPoint destination(curNode.stage_number + 1, 0);
  const unsigned dsize = get_size(destination.stage_number);

  for (; destination.point_index != dsize; ++destination.point_index)
    dijkstra.link(destination, curNode, distance(curNode, destination));
}

void 
TaskDijkstra::add_start_edges(const SearchPoint &currentLocation)
{
  // need to remove dummy first point
  dijkstra.pop();

  ScanTaskPoint destination(active_stage, 0);
  const unsigned dsize = get_size(destination.stage_number);

  for (; destination.point_index != dsize; ++destination.point_index)
    dijkstra.link(destination, destination,
                  distance(destination, currentLocation));
}

bool
TaskDijkstra::run()
{
  const bool retval = distance_general();
  if (retval)
    save();
  dijkstra.clear();
  return retval;
}
