// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class FileType : uint8_t {
  UNKNOWN,
  AIRSPACE,
  RASP,     // RASP before WAYPOINT, so "-rasp.dat" overrides ".dat"
  WAYPOINT,
  WAYPOINTDETAILS,
  MAP,
  FLARMNET,
  FLARMDB,
  IGC,
  NMEA,
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

/**
 * Guess the #FileType of a data file from its basename, using the same
 * rules as repository downloads and data-layout migration (enum order
 * matters, e.g. RASP before WAYPOINT for "*.dat").
 */
[[gnu::pure]]
FileType ClassifyDataFilename(const char *filename) noexcept;

/**
 * Subdirectory for a data file during layout migration (typed content
 * plus built-in files such as repository, notams.json, app logs).
 */
[[gnu::pure]]
AllocatedPath GetLayoutSubdirForFilename(const char *filename) noexcept;

/**
 * True for repository manifests and other files stored under product
 * cache/ on desktop (Android uses #GetCachePath() instead).
 */
[[gnu::pure]]
bool IsCacheLayoutFilename(const char *filename) noexcept;
