// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"
#include "Parser.hpp"
#include "net/http/Features.hpp"
#include "net/http/DownloadManager.hpp"
#include "system/Path.hpp"
#include "io/FileLineReader.hpp"
#include "LocalPath.hpp"

#include "util/StringFormat.hpp"
#include "util/StringCompare.hxx"
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

std::vector<RepositoryLink>
GetUserRepositories()
{
  std::vector<RepositoryLink> result;

  const char *src = Profile::Get(ProfileKeys::UserRepositoriesList);
  if (src == nullptr || *src == '\0') return result;

  int index = 1;
  for (auto i : TIterableSplitString(src, '|')) {
    if (i.empty()) continue;
    char filename[32];
    StringFormat(filename, std::size(filename), "user_repository_%d", index++);
    result.push_back({std::string{i}, std::string{filename}});
  }

  return result;
}

bool
IsUserRepositoryFile(std::string_view name) noexcept
{
  return StringStartsWith(name.data(), "user_repository_");
}

void
LoadAllRepositories(FileRepository &repository)
{
  try {
    FileLineReaderA reader(LocalPath("repository"));
    ParseFileRepository(repository, reader);
  } catch (const std::runtime_error &) {
    /* not yet downloaded - ignore */
  }

  for (const auto &repo : GetUserRepositories()) {
    try {
      FileLineReaderA reader(LocalPath(repo.filename.c_str()));
      ParseFileRepository(repository, reader);
    } catch (const std::runtime_error &) {
      /* not yet downloaded - ignore */
    }
  }
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
    for (const auto &repo : GetUserRepositories())
      Net::DownloadManager::Enqueue(repo.uri.c_str(), Path(repo.filename.c_str()));
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
    for (const auto &repo : GetUserRepositories()) {
      try {
        if (DownloadFileModal(_("Updating repository"),
                              repo.uri.c_str(), repo.filename.c_str()) == nullptr)
          return; /* cancelled */
      } catch (...) {
        ShowError(std::current_exception(), _("Updating repository"));
      }
    }
  }
}

#endif
