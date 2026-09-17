// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Route/WaypointReachability.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(5);

  ok1(!IsReachable(WaypointReachability::INVALID));
  ok1(!IsReachable(WaypointReachability::UNREACHABLE));
  ok1(IsReachable(WaypointReachability::STRAIGHT));
  ok1(IsReachable(WaypointReachability::TERRAIN));

  WaypointReach reach;
  ok1(!reach.IsReachable());

  return exit_status();
}