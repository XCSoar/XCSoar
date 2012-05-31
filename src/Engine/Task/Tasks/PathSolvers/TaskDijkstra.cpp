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

TaskDijkstra::TaskDijkstra(OrderedTask& _task, bool _is_min):
  NavDijkstra(0),
  task(_task),
  is_min(_is_min)
{
}

bool
TaskDijkstra::RefreshTask()
{
  const unsigned task_size = task.TaskSize();
  if (task_size < 2 || task_size > MAX_STAGES)
    return false;

  SetStageCount(task_size);

  active_stage = task.GetActiveTaskPointIndex();

  for (unsigned stage = 0; stage != num_stages; ++stage)
    sp_sizes[stage] = task.get_tp_search_points(stage).size();

  return true;
}

const SearchPoint &
TaskDijkstra::GetPoint(const ScanTaskPoint sp) const
{
  return task.get_tp_search_points(sp.GetStageNumber())[sp.GetPointIndex()];
}

void
TaskDijkstra::AddEdges(const ScanTaskPoint curNode)
{
  ScanTaskPoint destination(curNode.GetStageNumber() + 1, 0);
  const unsigned dsize = GetStageSize(destination.GetStageNumber());

  unsigned first_distance;

  for (const ScanTaskPoint end(destination.GetStageNumber(), dsize);
       destination != end; destination.IncrementPointIndex()) {
    unsigned distance = CalcDistance(curNode, destination);

    if (is_min) {
      /* This is a kludge to avoid rounding errors for the finish
         line: due to rounding errors, the outer edge of the finish
         line was sometimes preferred if the previous turn point was a
         cylinder.  This kludge checks if the first point on the
         boundary is only slightly larger than the following ones, and
         adjusts them.  It assumes that the middle point comes first
         in ObservationZone::GetBoundary() and should be preferred,
         and assumes that 0.1% difference is negligible.  The real
         problem is that the cylinder's GetBoundary() returns an
         approximation which doesn't have enough points. */

      if (destination.GetPointIndex() == 0)
        /* remember the first distance (the one that points to the
           center of the finish line) */
        first_distance = distance;
      else if (distance <= first_distance &&
               distance > (first_distance * 1023u) / 1024u)
        /* this distance is just slightly smaller (i.e. better) than
           the first one; put it back */
        distance = first_distance + 1;
    }

    Link(destination, curNode, distance);
  }
}

void 
TaskDijkstra::AddStartEdges(const SearchPoint &currentLocation)
{
  ScanTaskPoint destination(active_stage, 0);
  const unsigned dsize = GetStageSize(active_stage);

  for (const ScanTaskPoint end(active_stage, dsize);
       destination != end; destination.IncrementPointIndex())
    LinkStart(destination, CalcDistance(destination, currentLocation));
}

bool
TaskDijkstra::Run()
{
  solution_valid = false;
  const bool retval = DistanceGeneral() && solution_valid;
  dijkstra.Clear();
  return retval;
}
