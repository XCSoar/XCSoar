// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

// IWYU pragma: begin_exports
#include "Profile/Keys.hpp"
#include "Profile/ProfileMap.hpp"
// IWYU pragma: end_exports

#include <string_view>
#include <vector>

class Path;
class AllocatedPath;

namespace Profile {

/**
 * Returns the absolute path of the current profile file.
 */
[[gnu::pure]]
Path
GetPath() noexcept;

/**
 * Returns the absolute path of the default profile file. Unlike
 * GetPath(), this is available before the profile has been loaded.
 */
[[gnu::pure]]
AllocatedPath
GetDefaultPath() noexcept;

/**
 * Returns the profile file which was used most recently, i.e. the
 * newest one in the profile directory.  The startup dialog touches a
 * profile when it is selected, so this is the one it will preselect.
 * Falls back to GetDefaultPath() if there is no profile at all.
 */
AllocatedPath
GetMostRecentPath() noexcept;

/**
 * Loads the profile files
 */
void
Load() noexcept;

/**
 * Loads the given profile file
 */
void
LoadFile(Path path) noexcept;

/**
 * Saves the profile into the profile files
 *
 * Errors will be caught and logged.
 */
void
Save() noexcept;

/**
 * Saves the profile into the given profile file
 */
void
SaveFile(Path path);

/**
 * Sets the profile files to load when calling Load()
 * @param override nullptr or file to load when calling Load()
 */
void
SetFiles(Path override_path) noexcept;

/**
 * Reads a configured path from the profile, and expands it with
 * ExpandLocalPath().
 *
 * @param value a buffer which can store at least MAX_PATH
 * characters
 */
[[gnu::pure]]
AllocatedPath
GetPath(std::string_view key) noexcept;

std::vector<AllocatedPath> GetMultiplePaths(std::string_view key,
                                            const char *patterns);

void
SetPath(std::string_view key, Path value) noexcept;

[[gnu::pure]]
bool
GetPathIsEqual(std::string_view key, Path value) noexcept;

} // namespace Profile
