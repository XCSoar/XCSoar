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
#include "Task/Ordered/OrderedTask.hpp"
#include "Geo/SearchPointVector.hpp"

TaskDijkstra::TaskDijkstra(bool _is_min)
  :NavDijkstra(0),
   is_min(_is_min)
{
}

bool
TaskDijkstra::RefreshTask(const OrderedTask &task)
{
  const unsigned task_size = task.TaskSize();
  if (task_size < 2 || task_size > MAX_STAGES)
    return false;

  SetStageCount(task_size);

  active_stage = task.GetActiveTaskPointIndex();

  for (unsigned stage = 0; stage != num_stages; ++stage)
    boundaries[stage] = &task.GetPointSearchPoints(stage);

  return true;
}

inline unsigned
TaskDijkstra::GetStageSize(const unsigned stage) const
{
  assert(stage < num_stages);

  return boundaries[stage]->size();
}

const SearchPoint &
TaskDijkstra::GetPoint(const ScanTaskPoint sp) const
{
  return (*boundaries[sp.GetStageNumber()])[sp.GetPointIndex()];
}

void
TaskDijkstra::AddEdges(const ScanTaskPoint curNode)
{
  ScanTaskPoint destination(curNode.GetStageNumber() + 1, 0);
  const unsigned dsize = GetStageSize(destination.GetStageNumber());

  for (const ScanTaskPoint end(destination.GetStageNumber(), dsize);
       destination != end; destination.IncrementPointIndex())
    Link(destination, curNode, CalcDistance(curNode, destination));
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
  const bool retval = DistanceGeneral() == SolverResult::VALID;
  dijkstra.Clear();
  return retval;
}
