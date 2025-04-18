// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class WaypointFileType: uint8_t;
struct zzip_dir;
class Path;
class Waypoints;
class WaypointFactory;
class ProgressListener;

/**
 * Throws on error.
 */
void
ReadWaypointFile(Path path, WaypointFileType file_type,
                 Waypoints &way_points,
                 WaypointFactory factory, ProgressListener &progress);

/**
 * Throws on error.
 */
void
ReadWaypointFile(Path path, Waypoints &way_points,
                 WaypointFactory factory, ProgressListener &progress);

/**
 * Throws on error.
 */
void
ReadWaypointFile(struct zzip_dir *dir, const char *path,
                 WaypointFileType file_type, Waypoints &way_points,
                 WaypointFactory factory, ProgressListener &progress);
