// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "net/http/DownloadManager.hpp"
#include "system/Path.hpp"

#include <tchar.h>

static bool repository_downloaded = false;

void
EnqueueRepositoryDownload(bool force)
{
  if (repository_downloaded && !force)
    return;

  repository_downloaded = true;
  
  // Use new fallback API with multiple repository URLs
  Net::DownloadManager::EnqueueWithFallback({
    "http://download.xcsoar.org/repository", // Primary, managed by @scottp
    "http://download.xcsoar.net/repository", // Fallback, managed by @lordfolken
    "http://download.xcsoar.de/repository" // Fallback, managed by @yorickreum
  }, Path(_T("repository")));
}
