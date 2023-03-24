// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskDijkstraMax.hpp"

bool
TaskDijkstraMax::DistanceMax() noexcept
{
  dijkstra.Clear();
  dijkstra.Reserve(256);
  AddZeroStartEdges();
  return Run();
}
