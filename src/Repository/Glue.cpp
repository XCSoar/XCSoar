// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "net/http/Features.hpp"
#include "net/http/DownloadManager.hpp"
#include "system/Path.hpp"

#include "util/StringFormat.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "util/IterableSplitString.hxx"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "Dialogs/DownloadFileModal.hpp"
#include "Dialogs/Error.hpp"
#include "Language/Language.hpp"
#endif

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
EnqueueRepositoryDownload(bool force, bool main_repo, bool user_repo)
{
  if (main_repo) {
    if (!repository_downloaded || force) {
      repository_downloaded = true;
      Net::DownloadManager::Enqueue(REPOSITORY_URI, Path("repository"));
    }
  }

  // Enqueue additional user-defined repository URIs, if set
  if (user_repo) {
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
}

#ifdef HAVE_DOWNLOAD_MANAGER

void
DownloadRepositoriesModal(bool main_repo, bool user_repo)
{
  if (main_repo) {
    try {
      if (DownloadFileModal(_("Updating repository"), REPOSITORY_URI, "repository") == nullptr)
        return; /* cancelled */
    } catch (...) {
      ShowError(std::current_exception(), _("Updating repository"));
    }

    repository_downloaded = true;
  }

  if (user_repo) {
    const std::vector<std::string> uris = GetUserRepositoryURIs();
    char filename[32];
    int user_repository_index = 1;
    for (const auto &uri : uris) {
      if (uri.empty())
        continue;
      StringFormat(filename, sizeof(filename), "user_repository_%d",
                   user_repository_index++);
      try {
        if (DownloadFileModal(_("Updating repository"), uri.c_str(), filename) == nullptr)
          return; /* cancelled */
      } catch (...) {
        ShowError(std::current_exception(), _("Updating repository"));
      }
    }
  }
}

#endif
