// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

// IWYU pragma: begin_exports
#include "Profile/Keys.hpp"
#include "Profile/ProfileMap.hpp"
// IWYU pragma: end_exports

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
GetPath(const char *key) noexcept;

void
SetPath(const char *key, Path value) noexcept;

[[gnu::pure]]
bool
GetPathIsEqual(const char *key, Path value) noexcept;

} // namespace Profile
