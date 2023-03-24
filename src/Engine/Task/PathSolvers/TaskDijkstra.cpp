// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskDijkstra.hpp"
#include "Geo/SearchPointVector.hpp"

TaskDijkstra::TaskDijkstra(bool _is_min) noexcept
  :NavDijkstra(0),
   is_min(_is_min)
{
}

inline unsigned
TaskDijkstra::GetStageSize(const unsigned stage) const noexcept
{
  assert(stage < num_stages);

  return boundaries[stage]->size();
}

const SearchPoint &
TaskDijkstra::GetPoint(const ScanTaskPoint sp) const noexcept
{
  return (*boundaries[sp.GetStageNumber()])[sp.GetPointIndex()];
}

void
TaskDijkstra::AddEdges(const ScanTaskPoint curNode) noexcept
{
  ScanTaskPoint destination(curNode.GetStageNumber() + 1, 0);
  const unsigned dsize = GetStageSize(destination.GetStageNumber());

  for (const ScanTaskPoint end(destination.GetStageNumber(), dsize);
       destination != end; destination.IncrementPointIndex())
    Link(destination, curNode, CalcDistance(curNode, destination));
}

void
TaskDijkstra::AddZeroStartEdges() noexcept
{
  const unsigned stage = 0;
  ScanTaskPoint destination(stage, 0);
  const unsigned dsize = GetStageSize(stage);

  /* only the first start edge is really going to be zero; all
     following edges will be incremented, to add some bias preferring
     the first point which is usually the reference point of the
     observation zone; this prevents very rare miscalculations, which
     should never occur in real flights, but can fail our unit tests
     with synthetic input values */
  value_type value = 0;

  for (const ScanTaskPoint end(stage, dsize);
       destination != end; destination.IncrementPointIndex())
    LinkStart(destination, value++);
}

void
TaskDijkstra::AddStartEdges(unsigned stage,
                            const SearchPoint &currentLocation) noexcept
{
  assert(currentLocation.IsValid());

  ScanTaskPoint destination(stage, 0);
  const unsigned dsize = GetStageSize(stage);

  for (const ScanTaskPoint end(stage, dsize);
       destination != end; destination.IncrementPointIndex())
    LinkStart(destination, CalcDistance(destination, currentLocation));
}

bool
TaskDijkstra::Run() noexcept
{
  const bool retval = DistanceGeneral() == SolverResult::VALID;
  dijkstra.Clear();
  return retval;
}
