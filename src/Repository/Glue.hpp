// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AvailableFile.hpp"
#include "net/http/Features.hpp"
#include "system/Path.hpp"

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
 * Relative download path for a repository file (for
 * Net::DownloadManager::Enqueue()).
 */
AllocatedPath
GetFileDownloadRelativePath(const AvailableFile &file) noexcept;

/**
 * Return true when the local copy of @p file is older than the
 * repository metadata (same rule as the download manager UI).
 */
[[gnu::pure]]
bool
IsRemoteFileOutOfDate(const AvailableFile &file) noexcept;

/**
 * Queue a repository file for download.
 */
void
EnqueueRemoteFileDownload(const AvailableFile &file) noexcept;

/**
 * Base file name of the configured RASP file, or nullptr.
 */
[[gnu::pure]]
AllocatedPath
GetConfiguredRaspFileName() noexcept;

[[gnu::pure]]
const AvailableFile *
FindConfiguredRaspRemoteFile(const FileRepository &repository) noexcept;

[[gnu::pure]]
bool
IsConfiguredRaspOutOfDate(const FileRepository &repository) noexcept;

/**
 * Enqueue a download of the configured RASP file when it is listed in
 * the repository and out of date locally.
 *
 * @return true if a download was queued
 */
bool
EnqueueConfiguredRaspUpdate(const FileRepository &repository) noexcept;

/**
 * Enqueue a download of the configured RASP file when it is listed in
 * the repository, even when the local copy is up to date.
 *
 * @return true if a download was queued
 */
bool
EnqueueConfiguredRaspDownload(const FileRepository &repository) noexcept;

/**
 * Download the main repository and all user-defined repositories
 * modally, showing a #ProgressDialog for each one.  Errors are
 * reported to the user.  Call this after the user changes the
 * repository URL list in the configuration.
 */
void
DownloadRepositoriesModal(bool main_repo=true, bool user_repo=true);

#endif /* HAVE_DOWNLOAD_MANAGER */
