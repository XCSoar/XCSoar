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

#include "Util/AllocatedGrid.hxx"

extern "C" {
#include "tap.h"
}

int main(int argc, char **argv)
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
