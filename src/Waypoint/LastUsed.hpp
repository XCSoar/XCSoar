// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <list>

struct Waypoint;

class WaypointIDList: public std::list<unsigned> {};

namespace LastUsedWaypoints
{
  void Add(unsigned waypoint_id);
  void Add(const Waypoint &waypoint);

  const WaypointIDList &GetList();
}
