// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Repository/FileType.hpp"
#include "system/Path.hpp"

#include <vector>

struct MigrationEntry {
  AllocatedPath source_path;
  AllocatedPath destination_path;
  FileType type = FileType::UNKNOWN;

  MigrationEntry(AllocatedPath &&_source_path,
                 AllocatedPath &&_destination_path,
                 FileType _type) noexcept;
};

struct MigrationPlan {
  std::vector<MigrationEntry> moves;
  unsigned skipped_unknown = 0;
  unsigned skipped_conflicts = 0;
};

MigrationPlan
BuildMigrationPlan(Path root);

bool
UpdateProfileReferences(Path old_path, Path new_path);

/**
 * One-time migration of legacy flat XCSoarData files into typed
 * subdirectories. Rewrites path keys in the active profile only; other
 * profiles keep legacy paths and rely on ResolveLocalDataFile() when
 * loading files. Already-migrated data directories are skipped via a
 * marker file.
 */
void
MigrateDataLayoutToSubdirs() noexcept;
