// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "util/AllocatedGrid.hxx"

extern "C" {
#include "tap.h"
}

int main()
{
  plan_tests(54);

  AllocatedGrid<int> grid;

  grid.GrowPreserveFill(2, 2, 1);
  ok1(grid.Get(0, 0) == 1);
  ok1(grid.Get(1, 0) == 1);
  ok1(grid.Get(0, 1) == 1);
  ok1(grid.Get(1, 1) == 1);

  grid.GrowPreserveFill(3, 3, 2);
  ok1(grid.Get(0, 0) == 1);
  ok1(grid.Get(1, 0) == 1);
  ok1(grid.Get(2, 0) == 2);
  ok1(grid.Get(0, 1) == 1);
  ok1(grid.Get(1, 1) == 1);
  ok1(grid.Get(2, 1) == 2);
  ok1(grid.Get(0, 2) == 2);
  ok1(grid.Get(1, 2) == 2);
  ok1(grid.Get(2, 2) == 2);

  grid.GrowPreserveFill(4, 2, 3);
  ok1(grid.Get(0, 0) == 1);
  ok1(grid.Get(1, 0) == 1);
  ok1(grid.Get(2, 0) == 2);
  ok1(grid.Get(3, 0) == 3);
  ok1(grid.Get(0, 1) == 1);
  ok1(grid.Get(1, 1) == 1);
  ok1(grid.Get(2, 1) == 2);
  ok1(grid.Get(3, 1) == 3);

  grid.GrowPreserveFill(2, 4, 4);
  ok1(grid.Get(0, 0) == 1);
  ok1(grid.Get(1, 0) == 1);
  ok1(grid.Get(0, 1) == 1);
  ok1(grid.Get(1, 1) == 1);
  ok1(grid.Get(0, 2) == 4);
  ok1(grid.Get(1, 2) == 4);
  ok1(grid.Get(0, 3) == 4);
  ok1(grid.Get(1, 3) == 4);

  grid.GrowPreserveFill(5, 5, 5);
  ok1(grid.Get(0, 0) == 1);
  ok1(grid.Get(1, 0) == 1);
  ok1(grid.Get(2, 0) == 5);
  ok1(grid.Get(3, 0) == 5);
  ok1(grid.Get(4, 0) == 5);
  ok1(grid.Get(0, 1) == 1);
  ok1(grid.Get(1, 1) == 1);
  ok1(grid.Get(2, 1) == 5);
  ok1(grid.Get(3, 1) == 5);
  ok1(grid.Get(4, 1) == 5);
  ok1(grid.Get(0, 2) == 4);
  ok1(grid.Get(1, 2) == 4);
  ok1(grid.Get(2, 2) == 5);
  ok1(grid.Get(3, 2) == 5);
  ok1(grid.Get(4, 2) == 5);
  ok1(grid.Get(0, 3) == 4);
  ok1(grid.Get(1, 3) == 4);
  ok1(grid.Get(2, 3) == 5);
  ok1(grid.Get(3, 3) == 5);
  ok1(grid.Get(4, 3) == 5);
  ok1(grid.Get(0, 4) == 5);
  ok1(grid.Get(1, 4) == 5);
  ok1(grid.Get(2, 4) == 5);
  ok1(grid.Get(3, 4) == 5);
  ok1(grid.Get(4, 4) == 5);

  return exit_status();
}
