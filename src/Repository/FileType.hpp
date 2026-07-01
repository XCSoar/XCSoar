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
 * type (e.g. "*.openair\0*.txt\0*.air\0*.sua\0").  The list is terminated by
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
 * Classify a filename for typed layout placement.  Unlike
 * DetectFileTypeByFilename(), this only returns types with an actual
 * managed subdirectory.
 */
[[gnu::pure]]
FileType ClassifyDataFilename(const char *filename) noexcept;

/**
 * Return the layout subdirectory for a filename, or nullptr if this
 * filename should remain in the data root.
 */
[[gnu::pure]]
AllocatedPath GetLayoutSubdirForFilename(const char *filename) noexcept;

[[gnu::pure]]
bool IsCacheLayoutFilename(const char *filename) noexcept;

/**
 * Detect the unique #FileType for a filename.
 *
 * Exact-match patterns win.  If multiple wildcard-only file types
 * match, then FileType::UNKNOWN is returned.
 */
[[gnu::pure]]
FileType DetectFileTypeByFilename(const char *filename) noexcept;

/**
 * Parse a repository file ``type = …`` value into a #FileType.
 * Returns FileType::UNKNOWN for unrecognised values.
 */
[[gnu::pure]]
FileType FileTypeFromRepositoryString(const char *type) noexcept;
