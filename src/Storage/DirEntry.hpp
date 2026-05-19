// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * A single file or directory entry with optional metadata.
 */
struct DirEntry {
  std::string name;
  bool is_directory;
  /** File size in bytes (empty for directories or when unavailable). */
  std::optional<uint64_t> size;
  /** Last-modified time in milliseconds since Unix epoch. */
  std::optional<int64_t> last_modified_ms;
};

/**
 * List files and directories in a local filesystem directory,
 * populating size and last-modified metadata from stat().
 */
[[nodiscard]]
std::vector<DirEntry>
ListDirEntries(Path dir);
