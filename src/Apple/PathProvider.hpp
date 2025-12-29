// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

namespace Apple {

/**
 * Returns the relative directory name for application data within the
 * user's home directory (e.g. "Documents/XCSoar").
 */
Path
GetDataPathInHome() noexcept;

/**
 * Ensure the specified data path exists.
 *
 * @return true if the directory exists or was created successfully.
 */
bool
EnsureDataPathExists(Path path) noexcept;

} // namespace Apple
