// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Waypoints;
class ProgressListener;
class TLineReader;

namespace WaypointDetails {

/**
 * Opens the airfield details file and parses it
 *
 * Throws on error
 */
void
ReadFile(TLineReader &reader, Waypoints &way_points,
         ProgressListener &progress);

/**
 * Throws on error
 */
void
ReadFileFromProfile(Waypoints &way_points,
                    ProgressListener &progress);

} // namespace WaypointDetails
