// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <functional>

namespace DirectoryUtil {

/**
 * Visit files matching the pattern in the specified directory.
 *
 * The callback receives the full path and the file name.
 */
void
VisitSpecificFiles(Path directory, const char *pattern,
                   std::function<void(Path, Path)> visitor);

/**
 * Delete all files matching the pattern, except those for which keep()
 * returns true.
 *
 * @return number of deleted files
 */
unsigned
DeleteSpecificFilesExcept(Path directory, const char *pattern,
                          std::function<bool(Path, Path)> keep);

} // namespace DirectoryUtil

