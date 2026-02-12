// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class FileType : uint8_t {
  UNKNOWN,
  AIRSPACE,
  WAYPOINT,
  WAYPOINTDETAILS,
  MAP,
  FLARMNET,
  FLARMDB,
  IGC,
  NMEA,
  RASP,
  XCI,
  TASK,
  CHECKLIST,
  PROFILE,
  PLANE,
  COUNT,
};

class AllocatedPath;

AllocatedPath GetFileTypeDefaultDir(const FileType file_type);

/**
 * Return NUL-separated list of file glob patterns for the given
 * type (e.g. "*.txt\0*.air\0*.sua\0").  The list is terminated by
 * an empty pattern.
 */
const char *GetFileTypePatterns(const FileType file_type) noexcept;
