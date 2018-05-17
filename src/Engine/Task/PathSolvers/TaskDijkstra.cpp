/* Copyright_License {

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

#include "TaskDijkstra.hpp"
#include "Geo/SearchPointVector.hpp"

TaskDijkstra::TaskDijkstra(bool _is_min)
  :NavDijkstra(0),
   is_min(_is_min)
{
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
TaskDijkstra::AddZeroStartEdges()
{
  const unsigned stage = 0;
  ScanTaskPoint destination(stage, 0);
  const unsigned dsize = GetStageSize(stage);

  for (const ScanTaskPoint end(stage, dsize);
       destination != end; destination.IncrementPointIndex())
    LinkStart(destination, 0);
}

void 
TaskDijkstra::AddStartEdges(unsigned stage, const SearchPoint &currentLocation)
{
  assert(currentLocation.IsValid());

  ScanTaskPoint destination(stage, 0);
  const unsigned dsize = GetStageSize(stage);

  for (const ScanTaskPoint end(stage, dsize);
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
