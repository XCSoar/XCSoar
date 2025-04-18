// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Waypoint/Origin.hpp"

struct Waypoint;
class Waypoints;
class BufferedOutputStream;

void
WriteCup(BufferedOutputStream &writer, const Waypoint &wp);

void
WriteCup(BufferedOutputStream &writer, const Waypoints &waypoints,
         WaypointOrigin origin);
