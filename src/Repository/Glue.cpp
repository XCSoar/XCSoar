// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFilePath.hpp"
#include "FileType.hpp"
#include "FileRepository.hpp"
#include "Glue.hpp"
#include "Parser.hpp"
#include "LocalPath.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "net/http/Features.hpp"
#include "net/http/DownloadManager.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "io/FileLineReader.hpp"
#include "time/BrokenDateTime.hpp"

#include "util/StringFormat.hpp"
#include "util/StringCompare.hxx"
#include "util/IterableSplitString.hxx"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "Dialogs/DownloadFileModal.hpp"
#include "Dialogs/Error.hpp"
#include "Language/Language.hpp"
#endif

#include <string_view>
#include <vector>

#define REPOSITORY_URI "https://download.xcsoar.org/repository"

static constexpr std::string_view USER_REPOSITORY_FILE_PREFIX{
    "user_repository_"};

static bool repository_downloaded = false;
static bool user_repository_downloaded = false;

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

void
PurgeChangedUserRepositoryFiles(const char *old_list,
                                const char *new_list) noexcept
{
  const auto next_token = [](std::string_view &s) noexcept -> std::string_view {
    while (!s.empty()) {
      const auto pos = s.find('|');
      std::string_view token;
      if (pos == std::string_view::npos) {
        token = s;
        s = {};
      } else {
        token = s.substr(0, pos);
        s = s.substr(pos + 1);
      }
      if (!token.empty())
        return token;
    }
    return {};
  };

  std::string_view old_remaining{old_list != nullptr ? old_list : ""};
  std::string_view new_remaining{new_list != nullptr ? new_list : ""};

  for (int i = 1; ; ++i) {
    const auto old_entry = next_token(old_remaining);
    if (old_entry.empty())
      break;
    const auto new_entry = next_token(new_remaining);
    if (old_entry != new_entry) {
      char filename[32];
      StringFormat(filename, std::size(filename), "user_repository_%d", i);
      File::Delete(ResolveRepositoryDataPath(filename));
    }
  }
}

bool
IsUserRepositoryFile(std::string_view name) noexcept
{
  return name.size() >= USER_REPOSITORY_FILE_PREFIX.size() &&
         StringIsEqual(name.data(), USER_REPOSITORY_FILE_PREFIX.data(),
                       USER_REPOSITORY_FILE_PREFIX.size());
}

void
LoadAllRepositories(FileRepository &repository)
{
  try {
    FileLineReaderA reader(ResolveRepositoryDataPath("repository"));
    ParseFileRepository(repository, reader);
  } catch (const std::runtime_error &) {
    /* not yet downloaded - ignore */
  }

  for (const auto &repo : GetUserRepositories()) {
    try {
      FileLineReaderA reader(ResolveRepositoryDataPath(repo.filename.c_str()));
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
      const auto path = RepositoryDownloadRelativePath("repository");
      Net::DownloadManager::Enqueue(REPOSITORY_URI, Path(path.c_str()));
    }
  }

  // Enqueue additional user-defined repository URIs, if set
  if (user_repo) {
    if (!user_repository_downloaded || force) {
      user_repository_downloaded = true;
      for (const auto &repo : GetUserRepositories())
        {
          const auto path =
            RepositoryDownloadRelativePath(repo.filename.c_str());
          Net::DownloadManager::Enqueue(repo.uri.c_str(), Path(path.c_str()));
        }
    }
  }
}

#ifdef HAVE_DOWNLOAD_MANAGER

AllocatedPath
GetFileDownloadRelativePath(const AvailableFile &file) noexcept
{
  const auto base = file.GetName();
  if (base == nullptr || *base == '\0')
    return nullptr;

  const AllocatedPath subdir = GetFileTypeDefaultDir(file.type);
  if (subdir == nullptr)
    return AllocatedPath(base);

  return AllocatedPath::Build(subdir, Path(base));
}

bool
IsRemoteFileOutOfDate(const AvailableFile &file) noexcept
{
  const auto path = LocalPath(GetFileDownloadRelativePath(file));
  if (path == nullptr)
    return false;

  if (!File::Exists(path))
    return true;

  const BrokenDate local_changed =
    BrokenDateTime{File::GetLastModification(path)};
  return local_changed < file.update_date;
}

void
EnqueueRemoteFileDownload(const AvailableFile &file) noexcept
{
  if (!Net::DownloadManager::IsAvailable())
    return;

  const auto relative_path = GetFileDownloadRelativePath(file);
  if (relative_path == nullptr)
    return;

  const AllocatedPath subdir = GetFileTypeDefaultDir(file.type);
  if (subdir != nullptr) {
    const auto dest_path = LocalPath(subdir);
    Directory::CreateRecursive(dest_path);
    if (!Directory::Exists(dest_path))
      return;
  }

  Net::DownloadManager::Enqueue(file.GetURI(), Path(relative_path.c_str()));
}

AllocatedPath
GetConfiguredRaspFileName() noexcept
{
  auto path = Profile::GetPath(ProfileKeys::RaspFile);
  if (path == nullptr)
    path = ResolveTypedDataFilePath(FileType::RASP, RASP_FILENAME);
  if (path == nullptr)
    return nullptr;

  const Path base = path.GetBase();
  if (base == nullptr || base.empty())
    return nullptr;

  return AllocatedPath(base.c_str());
}

const AvailableFile *
FindConfiguredRaspRemoteFile(const FileRepository &repository) noexcept
{
  const auto name = GetConfiguredRaspFileName();
  if (name == nullptr)
    return nullptr;

  return repository.FindByName(name.c_str());
}

bool
IsConfiguredRaspOutOfDate(const FileRepository &repository) noexcept
{
  const AvailableFile *remote = FindConfiguredRaspRemoteFile(repository);
  if (remote == nullptr)
    return false;

  return IsRemoteFileOutOfDate(*remote);
}

bool
EnqueueConfiguredRaspUpdate(const FileRepository &repository) noexcept
{
  const AvailableFile *remote = FindConfiguredRaspRemoteFile(repository);
  if (remote == nullptr || !IsRemoteFileOutOfDate(*remote))
    return false;

  EnqueueRemoteFileDownload(*remote);
  return true;
}

bool
EnqueueConfiguredRaspDownload(const FileRepository &repository) noexcept
{
  const AvailableFile *remote = FindConfiguredRaspRemoteFile(repository);
  if (remote == nullptr)
    return false;

  EnqueueRemoteFileDownload(*remote);
  return true;
}

#endif /* HAVE_DOWNLOAD_MANAGER */

#ifdef HAVE_DOWNLOAD_MANAGER

void
DownloadRepositoriesModal(bool main_repo, bool user_repo)
{
  if (main_repo) {
    try {
      const auto path = RepositoryDownloadRelativePath("repository");
      if (DownloadFileModal(_("Updating repository"), REPOSITORY_URI,
                            path.c_str()) == nullptr)
        return; /* cancelled */
      repository_downloaded = true;
    } catch (...) {
      ShowError(std::current_exception(), _("Updating repository"));
    }
  }

  if (user_repo) {
    for (const auto &repo : GetUserRepositories()) {
      try {
        const auto path =
          RepositoryDownloadRelativePath(repo.filename.c_str());
        if (DownloadFileModal(_("Updating repository"),
                              repo.uri.c_str(), path.c_str()) == nullptr)
          return; /* cancelled */
      } catch (...) {
        ShowError(std::current_exception(), _("Updating repository"));
      }
    }
    user_repository_downloaded = true;
  }
}

bool
FileTypeSupportsDownload(FileType type) noexcept
{
  return type != FileType::IGC && type != FileType::UNKNOWN &&
         Net::DownloadManager::IsAvailable();
}

#endif
