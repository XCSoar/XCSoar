// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Ptr.hpp"
#include "GlideSolvers/GlideResult.hpp"

struct AlternatePoint {
  WaypointPtr waypoint;

  GlideResult solution;

  explicit AlternatePoint(const WaypointPtr &_waypoint)
    :waypoint(_waypoint)
  {
    solution.Reset();
  }

  AlternatePoint(const WaypointPtr &_waypoint, const GlideResult &_solution)
    :waypoint(_waypoint), solution(_solution) {}
};
