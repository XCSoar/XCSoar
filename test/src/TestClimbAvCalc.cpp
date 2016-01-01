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

#include "Computer/ClimbAverageCalculator.hpp"
#include "TestUtil.hpp"

#include <cstdio>

static void
TestBasic()
{
  ClimbAverageCalculator c;
  c.Reset();

  double av;

  constexpr double AVERAGE_TIME = 30;

  // Test normal behavior
  c.GetAverage(0, 0, AVERAGE_TIME);
  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(i, i, AVERAGE_TIME);

  ok1(equals(av, 1.0));

  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(15 + i, 15 + i * 2, AVERAGE_TIME);

  ok1(equals(av, 1.5));

  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(30 + i, 45 + i * 2, AVERAGE_TIME);

  ok1(equals(av, 2.0));
}

static void
TestDuplicateTimestamps()
{
  ClimbAverageCalculator c;
  double av;

  constexpr double AVERAGE_TIME = 30;

  // Test time difference = zero behavior
  c.Reset();
  c.GetAverage(0, 0, AVERAGE_TIME);
  for (unsigned i = 1; i <= 15; i++)
    c.GetAverage(i, i, AVERAGE_TIME);

  for (unsigned i = 1; i <= 15; i++) {
    c.GetAverage(15 + i, 15 + i * 2, AVERAGE_TIME);
    c.GetAverage(15 + i, 15 + i * 2, AVERAGE_TIME);
    av = c.GetAverage(15 + i, 15 + i * 2, AVERAGE_TIME);
  }

  ok1(equals(av, 1.5));
}

static void
TestExpiration()
{
  ClimbAverageCalculator c;
  c.Reset();

  constexpr double AVERAGE_TIME = 30;


  // Test expiration for empty data
  ok1(c.Expired(0, 60));
  ok1(c.Expired(15, 60));

  // Add values and test non-expiration
  bool expired = false;
  for (unsigned i = 1; i <= 60; i++) {
    c.GetAverage(i, i, AVERAGE_TIME);
    expired = expired || c.Expired(i, 60);
  }

  ok1(!expired);

  // Test expiration with 30sec
  ok1(!c.Expired(89, 30));
  ok1(!c.Expired(90, 30));
  ok1(c.Expired(91, 30));

  // Test expiration with 60sec
  ok1(!c.Expired(119, 60));
  ok1(!c.Expired(120, 60));
  ok1(c.Expired(121, 60));

  // Time warp
  ok1(c.Expired(59, 60));
  ok1(!c.Expired(60, 60));
  ok1(!c.Expired(61, 60));
}

int main(int argc, char **argv)
{
  plan_tests(16);

  TestBasic();
  TestDuplicateTimestamps();
  TestExpiration();

  return exit_status();
}
