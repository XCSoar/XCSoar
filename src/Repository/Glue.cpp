// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "net/http/DownloadManager.hpp"
#include "system/Path.hpp"

#include "util/StringFormat.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "util/IterableSplitString.hxx"

#include <vector>

#define REPOSITORY_URI "http://download.xcsoar.org/repository"

static bool repository_downloaded = false;

std::vector<std::string> GetUserRepositoryURIs()
{
  std::vector<std::string> uris;

  const char *src = Profile::Get(ProfileKeys::UserRepositoriesList);
  if (src == nullptr) return uris;
  if (*src == '\0') return uris;

  for (auto i : TIterableSplitString(src, '|')) {
    if (i.empty()) continue;
    uris.emplace_back(i.data(), i.size());
  }

  return uris;
}

void
EnqueueRepositoryDownload(bool force)
{
  if (repository_downloaded && !force)
    return;

  repository_downloaded = true;
  Net::DownloadManager::Enqueue(REPOSITORY_URI, Path("repository"));

  // Enqueue additional user-defined repository URIs, if set
  char filename[32];
  std::vector<std::string> uris = GetUserRepositoryURIs();
  int user_repository_index = 1;
  for (const auto &uri : uris) {
    if (uri.empty())
      continue;
    StringFormat(filename, std::size(filename), "user_repository_%d",
                 user_repository_index++);
    Net::DownloadManager::Enqueue(uri.c_str(), Path(filename));
  }
}
