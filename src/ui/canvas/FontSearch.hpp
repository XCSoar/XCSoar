// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

namespace FontSearch {

/**
 * Find a file in a list of search paths.
 */
[[nodiscard]]
AllocatedPath
FindInSearchPaths(const char *const *search_paths, Path suffix) noexcept;

/**
 * Find the first existing file from a list of candidates. Relative
 * paths are resolved against the given search paths.
 */
[[nodiscard]]
AllocatedPath
FindFile(const char *const *search_paths, const char *const *list) noexcept;

} // namespace FontSearch
