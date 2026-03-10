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
  TASK,
  CHECKLIST,
  PROFILE,
  PLANE,
  XCI,
  LUA,
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

/**
 * Check whether a filename matches an exact (non-wildcard) pattern
 * for any #FileType.  Returns that type, or FileType::UNKNOWN if
 * no exact match exists.
 */
[[gnu::pure]]
FileType SpecialFilenameType(const char *filename) noexcept;

/**
 * Check whether a filename matches any of the glob patterns
 * for the given #FileType.
 */
[[gnu::pure]]
bool FilenameMatchesFileType(const char *filename,
                             FileType file_type) noexcept;
