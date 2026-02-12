// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/http/Features.hpp"

#include <vector>
#include <string>

/**
 * Get list of URIs of user repositories
 */
std::vector<std::string> GetUserRepositoryURIs();

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
