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

#include "ClimbAverageCalculator.hpp"
#include "TestUtil.hpp"

#include <cstdio>

static void
TestBasic()
{
  ClimbAverageCalculator c;
  fixed av;

  constexpr fixed AVERAGE_TIME = fixed(30);

  // Test normal behavior
  c.GetAverage(fixed_zero, fixed_zero, AVERAGE_TIME);
  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(fixed(i), fixed(i), AVERAGE_TIME);

  ok1(equals(av, 1.0));

  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(fixed(15 + i), fixed(15 + i * 2), AVERAGE_TIME);

  ok1(equals(av, 1.5));

  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(fixed(30 + i), fixed(45 + i * 2), AVERAGE_TIME);

  ok1(equals(av, 2.0));
}

static void
TestDuplicateTimestamps()
{
  ClimbAverageCalculator c;
  fixed av;

  constexpr fixed AVERAGE_TIME = fixed(30);

  // Test time difference = zero behavior
  c.Reset();
  c.GetAverage(fixed_zero, fixed_zero, AVERAGE_TIME);
  for (unsigned i = 1; i <= 15; i++)
    c.GetAverage(fixed(i), fixed(i), AVERAGE_TIME);

  for (unsigned i = 1; i <= 15; i++) {
    c.GetAverage(fixed(15 + i), fixed(15 + i * 2), AVERAGE_TIME);
    c.GetAverage(fixed(15 + i), fixed(15 + i * 2), AVERAGE_TIME);
    av = c.GetAverage(fixed(15 + i), fixed(15 + i * 2), AVERAGE_TIME);
  }

  ok1(equals(av, 1.5));
}

int main(int argc, char **argv)
{
  plan_tests(4);

  TestBasic();
  TestDuplicateTimestamps();

  return exit_status();
}
