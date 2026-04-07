// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/http/Features.hpp"

#include <string>
#include <string_view>
#include <vector>

struct FileRepository;

struct RepositoryLink {
  std::string uri;
  std::string filename;
};

/**
 * Get list of user repositories with their local filenames.
 */
std::vector<RepositoryLink> GetUserRepositories();

/**
 * Returns true if the given filename is a user repository file
 * (i.e. matches the "user_repository_*" pattern).
 */
[[gnu::pure]] bool IsUserRepositoryFile(std::string_view name) noexcept;

/**
 * Delete cached repository files for user repository entries that have
 * changed position or been removed, comparing @a old_list with @a new_list.
 * Call this before downloading new user repositories when the URL list changes.
 */
void PurgeChangedUserRepositoryFiles(const char *old_list,
                                     const char *new_list) noexcept;

/**
 * Load the main repository file and all user repository files into
 * @a repository.  Files not yet downloaded are silently skipped.
 */
void LoadAllRepositories(FileRepository &repository);

/**
 * Download the repository file.
 *
 * @param force if true, then download it even when this library
 * believes it is still fresh
 */
void
EnqueueRepositoryDownload(bool force=false, bool main_repo=true, bool user_repo=true);

#ifdef HAVE_DOWNLOAD_MANAGER
/**
 * Download the main repository and all user-defined repositories
 * modally, showing a #ProgressDialog for each one.  Errors are
 * reported to the user.  Call this after the user changes the
 * repository URL list in the configuration.
 */
void
DownloadRepositoriesModal(bool main_repo=true, bool user_repo=true);
#endif
