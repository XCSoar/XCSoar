// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskDijkstraMin.hpp"

bool
TaskDijkstraMin::DistanceMin(const SearchPoint &currentLocation) noexcept
{
  dijkstra.Clear();
  dijkstra.Reserve(256);

  if (currentLocation.IsValid()) {
    AddStartEdges(0, currentLocation);
  } else {
    AddZeroStartEdges();
  }

  return Run();
}

