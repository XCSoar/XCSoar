// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct Waypoint;
class Waypoints;

const Waypoint* lookup_waypoint(const Waypoints& waypoints, unsigned id);
bool SetupWaypoints(Waypoints &waypoints, const unsigned n=150);
