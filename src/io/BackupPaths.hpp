// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <string>
#include <string_view>

/**
 * True if @p path (archive-relative, '/' separators) is a cache,
 * log, or previous backup that should not be packed or restored.
 */
[[nodiscard]]
bool
IsBackupExcludedPath(std::string_view path) noexcept;

/**
 * Archive-relative path with '/' separators, or empty if @p path is
 * not below @p root.
 */
[[nodiscard]]
std::string
MakeArchiveNameIfUnderRoot(Path path, Path root) noexcept;
