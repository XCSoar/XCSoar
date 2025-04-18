// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Waypoints;
class ProgressListener;
class BufferedReader;

namespace WaypointDetails {

/**
 * Throws on error
 */
void
ReadFile(BufferedReader &reader, Waypoints &way_points);

/**
 * Opens the airfield details file and parses it
 *
 * Throws on error
 */
void
ReadFileFromProfile(Waypoints &way_points,
                    ProgressListener &progress);

} // namespace WaypointDetails
