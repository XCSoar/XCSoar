// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

namespace DataFileLayout {

/**
 * Determine the managed data subdirectory for a candidate data file.
 * Filename-based layout policy comes from Repository/FileType; ambiguous
 * root-level .txt files get an additional content sniff.
 */
AllocatedPath
GetLayoutSubdirForDataFile(Path path) noexcept;

/**
 * Move a root-level data file into its managed subdirectory if the target
 * location can be identified safely.  Returns the final path.
 */
AllocatedPath
RelocateRootDataFileToLayoutSubdir(Path path) noexcept;

} // namespace DataFileLayout
