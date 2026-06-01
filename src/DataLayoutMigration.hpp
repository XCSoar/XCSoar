// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * One-time migration of legacy flat XCSoarData files into typed
 * subdirectories.  On Android, cache-category files (repository,
 * notams.json, user_repository_*) go to #GetCachePath() instead of
 * product data cache/.  Rewrites path keys in the active profile only;
 * other profiles keep legacy paths and rely on ResolveLocalDataFile()
 * when loading files.
 *
 * Call after Profile::Load().  Already-migrated data directories are
 * skipped via a marker file.
 */
void
MigrateDataLayoutToSubdirs() noexcept;
