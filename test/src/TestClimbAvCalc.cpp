/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

  constexpr FloatDuration AVERAGE_TIME = std::chrono::seconds{30};

  // Test normal behavior
  c.GetAverage(TimeStamp{}, 0, AVERAGE_TIME);
  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(TimeStamp{std::chrono::seconds{i}}, i, AVERAGE_TIME);

  ok1(equals(av, 1.0));

  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(TimeStamp{std::chrono::seconds{15 + i}}, 15 + i * 2, AVERAGE_TIME);

  ok1(equals(av, 1.5));

  for (unsigned i = 1; i <= 15; i++)
    av = c.GetAverage(TimeStamp{std::chrono::seconds{30 + i}}, 45 + i * 2, AVERAGE_TIME);

  ok1(equals(av, 2.0));
}

static void
TestDuplicateTimestamps()
{
  ClimbAverageCalculator c;
  double av;

  constexpr FloatDuration AVERAGE_TIME = std::chrono::seconds{30};

  // Test time difference = zero behavior
  c.Reset();
  c.GetAverage(TimeStamp{}, 0, AVERAGE_TIME);
  for (unsigned i = 1; i <= 15; i++)
    c.GetAverage(TimeStamp{std::chrono::seconds{i}}, i, AVERAGE_TIME);

  for (unsigned i = 1; i <= 15; i++) {
    c.GetAverage(TimeStamp{std::chrono::seconds{15 + i}}, 15 + i * 2, AVERAGE_TIME);
    c.GetAverage(TimeStamp{std::chrono::seconds{15 + i}}, 15 + i * 2, AVERAGE_TIME);
    av = c.GetAverage(TimeStamp{std::chrono::seconds{15 + i}}, 15 + i * 2, AVERAGE_TIME);
  }

  ok1(equals(av, 1.5));
}

static void
TestExpiration()
{
  ClimbAverageCalculator c;
  c.Reset();

  constexpr FloatDuration AVERAGE_TIME = std::chrono::seconds{30};

  // Test expiration for empty data
  ok1(c.Expired(TimeStamp{}, std::chrono::minutes{1}));
  ok1(c.Expired(TimeStamp{std::chrono::seconds{15}}, std::chrono::minutes{1}));

  // Add values and test non-expiration
  bool expired = false;
  for (unsigned i = 1; i <= 60; i++) {
    c.GetAverage(TimeStamp{std::chrono::seconds{i}}, i, AVERAGE_TIME);
    expired = expired || c.Expired(TimeStamp{std::chrono::seconds{i}}, std::chrono::minutes{1});
  }

  ok1(!expired);

  // Test expiration with 30sec
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{89}}, std::chrono::seconds{30}));
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{90}}, std::chrono::seconds{30}));
  ok1(c.Expired(TimeStamp{std::chrono::seconds{91}}, std::chrono::seconds{30}));

  // Test expiration with 60sec
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{119}}, std::chrono::minutes{1}));
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{120}}, std::chrono::minutes{1}));
  ok1(c.Expired(TimeStamp{std::chrono::seconds{121}}, std::chrono::minutes{1}));

  // Time warp
  ok1(c.Expired(TimeStamp{std::chrono::seconds{59}}, std::chrono::minutes{1}));
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{60}}, std::chrono::minutes{1}));
  ok1(!c.Expired(TimeStamp{std::chrono::seconds{61}}, std::chrono::minutes{1}));
}

int main(int argc, char **argv)
{
  plan_tests(16);

  TestBasic();
  TestDuplicateTimestamps();
  TestExpiration();

  return exit_status();
}
