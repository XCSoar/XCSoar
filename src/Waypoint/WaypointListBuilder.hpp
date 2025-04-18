// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Engine/Task/Shapes/FAITrianglePointValidator.hpp"
#include "Engine/Waypoint/Ptr.hpp"

struct WaypointFilter;
class WaypointList;
class Waypoints;

class WaypointListBuilder final {
  const WaypointFilter &filter;
  const GeoPoint location;
  WaypointList &list;
  const FAITrianglePointValidator triangle_validator;

public:
  WaypointListBuilder(const WaypointFilter &_filter,
                      GeoPoint _location, WaypointList &_list,
                      OrderedTask *ordered_task,
                      unsigned ordered_task_index) noexcept
    :filter(_filter), location(_location), list(_list),
     triangle_validator(ordered_task, ordered_task_index) {}

  void Visit(const Waypoints &waypoints) noexcept;

  void operator()(const WaypointPtr &waypoint) noexcept;
};
