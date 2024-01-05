// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Factory.hpp"

class Waypoints;
class BufferedReader;

/**
 * @return true if the "Related Tasks" line was found, false if the
 * file contains no task
 *
 * Throws on error.
 */
bool ParseSeeYou(WaypointFactory factory, Waypoints &waypoints, BufferedReader &reader);
