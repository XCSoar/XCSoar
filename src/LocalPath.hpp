// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class Path;
class AllocatedPath;

namespace File {
  class Visitor;
}

/**
 * Determine the data path.
 *
 * Throws on error.
 */
void
InitialiseDataPath();

/**
 * Release resources obtained by InitialiseDataPath().
 */
void
DeinitialiseDataPath() noexcept;

/**
 * Create the primary data path;
 */
void
CreateDataPath();

/**
 * Changes the primary data path.  All other data paths found by
 * InitialiseDataPath() remain.
 */
void
SetPrimaryDataPath(Path path) noexcept;

/**
 * Sets the data path, replacing all data paths found by
 * InitialiseDataPath().
 */
void
SetSingleDataPath(Path path) noexcept;

/**
 * Returns the absolute path of the primary data directory.
 */
Path
GetPrimaryDataPath() noexcept;

/**
 * Gives the position of an XCSoar data file within the particular file
 * system of this host.
 * @param file The name of the file in question. Should not be equal to 
 *             'nullptr'.
 * @return The fully qualified path of file.
 */
AllocatedPath
LocalPath(Path file) noexcept;

AllocatedPath
LocalPath(const char *file) noexcept;

/**
 * Create a subdirectory of XCSoarData and return its absolute path.
 */
AllocatedPath
MakeLocalPath(const char *name);

/**
 * Return the portion of the specified path that is relative to the
 * primary data path.  Returns nullptr on failure (if the path is not
 * inside the primary data path).
 */
[[gnu::pure]]
Path
RelativePath(Path path) noexcept;

/**
 * Converts a file path by replacing %LOCAL_PATH% with the full pathname to
 * the XCSoarData folder
 * @param filein Pointer to the string to convert
 */
[[gnu::pure]]
AllocatedPath
ExpandLocalPath(Path src) noexcept;

/**
 * Converts a file path from full pathname to a shorter version with the
 * XCSoarData folder replaced by %LOCAL_PATH%
 * @param filein Pointer to the string to convert
 * @return the new path or nullptr if the given path cannot be contracted
 */
[[gnu::pure]]
AllocatedPath
ContractLocalPath(Path src) noexcept;

void VisitDataFiles(const char* filter, File::Visitor &visitor);

[[gnu::pure]]
Path
GetCachePath() noexcept;

[[gnu::pure]]
AllocatedPath
MakeCacheDirectory(const char *name) noexcept;
