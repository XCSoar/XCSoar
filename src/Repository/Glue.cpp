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
#include "ui/event/Notify.hpp"
#include "thread/Mutex.hxx"
#include "Weather/BackgroundDownloadProgress.hpp"
#include "UtilsSettings.hpp"
#include "LogFile.hpp"
#include "Message.hpp"
#include "Interface.hpp"
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

#include <set>
#include <string>
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

AllocatedPath
LocalPathForTypedName(const char *name, FileType type) noexcept
{
  if (name == nullptr)
    return nullptr;

  const AllocatedPath subdir = GetFileTypeDefaultDir(type);
  const AllocatedPath path = (subdir == nullptr)
    ? AllocatedPath(name)
    : AllocatedPath::Build(subdir, Path(name));
  return LocalPath(path);
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

AllocatedPath
LocalPathForRepositoryFile(const AvailableFile &file) noexcept
{
  return LocalPathForTypedName(file.GetName(), file.type);
}

bool
RepositoryFileCanDownload(const FileRepository &repository,
                          const char *name) noexcept
{
  return repository.FindByName(name) != nullptr;
}

bool
RepositoryFileUpdateAvailable(const FileRepository &repository,
                              const char *name) noexcept
{
  const AvailableFile *remote_file = repository.FindByName(name);
  if (remote_file == nullptr)
    return false;

  return IsRemoteFileOutOfDate(*remote_file);
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

bool
EnqueueRemoteFileDownload(const AvailableFile &file) noexcept
{
  if (!Net::DownloadManager::IsAvailable())
    return false;

  const auto relative_path = GetFileDownloadRelativePath(file);
  if (relative_path == nullptr)
    return false;

  const AllocatedPath subdir = GetFileTypeDefaultDir(file.type);
  if (subdir != nullptr) {
    const auto dest_path = LocalPath(subdir);
    Directory::CreateRecursive(dest_path);
    if (!Directory::Exists(dest_path))
      return false;
  }

  Net::DownloadManager::Enqueue(file.GetURI(), Path(relative_path.c_str()));
  return true;
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

/**
 * RASP forecasts are regenerated every day, but the repository
 * "update=" field is the static publication date of the product for
 * some providers (the ThermalMap entry has been frozen since 2023), so
 * IsRemoteFileOutOfDate() would never ask for a fresh forecast.  Treat
 * the local copy as out of date when it is missing or was last written
 * before today, which downloads at most once per calendar day.
 */
static bool
IsRaspFileOutOfDate(const AvailableFile &file) noexcept
{
  const auto path = LocalPath(GetFileDownloadRelativePath(file));
  if (path == nullptr)
    return false;

  if (!File::Exists(path))
    return true;

  const BrokenDate modified =
    BrokenDateTime{File::GetLastModification(path)};

  /* the system clock, not GPS time: this also runs at startup, before
     a fix is available */
  return IsRaspForecastOutOfDate(modified, BrokenDateTime::NowUTC(),
                                 file.update_date);
}

bool
EnqueueConfiguredRaspUpdate(const FileRepository &repository) noexcept
{
  const AvailableFile *remote = FindConfiguredRaspRemoteFile(repository);
  if (remote == nullptr || !IsRaspFileOutOfDate(*remote))
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
    bool user_repo_download_success = true;
    for (const auto &repo : GetUserRepositories()) {
      try {
        const auto path =
          RepositoryDownloadRelativePath(repo.filename.c_str());
        if (DownloadFileModal(_("Updating repository"),
                              repo.uri.c_str(), path.c_str()) == nullptr)
          return; /* cancelled */
      } catch (...) {
        user_repo_download_success = false;
        ShowError(std::current_exception(), _("Updating repository"));
      }
    }
    if (user_repo_download_success)
      user_repository_downloaded = true;
  }
}

bool
FileTypeSupportsDownload(FileType type) noexcept
{
  return type != FileType::IGC && type != FileType::UNKNOWN &&
         Net::DownloadManager::IsAvailable();
}

/**
 * After a repository index arrives, queue newer copies of files
 * already on disk. Missing files stay missing.
 * shortcut: listener stays up all session, so a later index refresh
 * updates too. Split it from File Manager if that double-reloads.
 */
class RepositoryAutoUpdate final : public Net::DownloadListener {
  struct Event {
    bool error;
    std::string path;
  };

  UI::Notify notify{[this]{ OnNotify(); }};
  Mutex mutex;
  std::vector<Event> events;
  std::set<std::string> queued;

  bool registered = false;
  bool progress = false;
  bool terrain_idle = false;
  bool stop = false;
  bool updated = false;

  void Push(bool error, Path path) noexcept {
    if (path == nullptr || path.empty())
      return;

    {
      const std::lock_guard lock{mutex};
      events.push_back({error, path.c_str()});
    }
    notify.SendNotification();
  }

  static bool IsIndex(Path path) noexcept {
    const Path name = path.GetBase();
    if (name == nullptr || name.empty())
      return false;

    return StringIsEqual(name.c_str(), "repository") ||
           IsUserRepositoryFile(name.c_str());
  }

  void EnqueueOutOfDate() noexcept {
    FileRepository repository;
    LoadAllRepositories(repository);

    for (const AvailableFile &file : repository) {
      const AllocatedPath relative = GetFileDownloadRelativePath(file);
      if (relative == nullptr)
        continue;

      const AllocatedPath local = LocalPath(relative);
      if (local == nullptr || !File::Exists(local) ||
          !IsRemoteFileOutOfDate(file))
        continue;

      const std::string key{relative.c_str()};
      if (queued.contains(key) || !EnqueueRemoteFileDownload(file))
        continue;

      queued.insert(key);
    }
  }

  void Show() noexcept {
    if (stop || !terrain_idle || queued.empty() || progress)
      return;

    BackgroundDownloadProgress::Get().Begin(_("Updating"));
    progress = true;
  }

  void Hide() noexcept {
    if (!progress)
      return;

    BackgroundDownloadProgress::Get().End();
    progress = false;
  }

  void OnNotify() noexcept {
    if (stop)
      return;

    std::vector<Event> batch;
    {
      const std::lock_guard lock{mutex};
      batch.swap(events);
    }

    bool index = false;
    std::vector<std::string> finished;
    for (const Event &event : batch) {
      const Path path{event.path.c_str()};
      if (IsIndex(path)) {
        index |= !event.error;
        continue;
      }

      if (queued.erase(event.path) == 0)
        continue;
      if (!event.error) {
        finished.push_back(event.path);
        updated = true;
      }
    }

    if (index)
      EnqueueOutOfDate();

    /* A map or airspace reload suspends threads and drops warnings.
       While flying, leave the new file on disk until the next start. */
    if (!finished.empty() &&
        !CommonInterface::Calculated().flight.flying) {
      try {
        ApplyConfiguredDataFiles(finished);
      } catch (...) {
        LogError(std::current_exception(),
                 "Failed to load downloaded file");
      }
    }

    if (!queued.empty()) {
      Show();
      return;
    }

    Hide();
    if (!updated)
      return;

    updated = false;
    Message::AddMessage(_("Data files have been updated"));
  }

public:
  void Start() noexcept {
    if (registered || stop || !Net::DownloadManager::IsAvailable())
      return;

    bool enabled = false;
    Profile::Get(ProfileKeys::RepositoryAutoUpdate, enabled);
    if (!enabled)
      return;

    Net::DownloadManager::AddListener(*this);
    registered = true;
    EnqueueRepositoryDownload(true);
  }

  void TerrainIdle() noexcept {
    terrain_idle = true;
    Show();
  }

  void Shutdown() noexcept {
    stop = true;
    queued.clear();
    {
      const std::lock_guard lock{mutex};
      events.clear();
    }

    if (registered) {
      Net::DownloadManager::RemoveListener(*this);
      registered = false;
    }

    notify.ClearNotification();
    Hide();
  }

  void OnDownloadAdded(Path, int64_t, int64_t) noexcept override {}

  void OnDownloadComplete(Path path_relative) noexcept override {
    Push(false, path_relative);
  }

  void OnDownloadError(Path path_relative,
                       std::exception_ptr) noexcept override {
    Push(true, path_relative);
  }
};

static RepositoryAutoUpdate &
GetRepositoryAutoUpdate() noexcept
{
  static RepositoryAutoUpdate instance;
  return instance;
}

void
StartRepositoryAutoUpdate() noexcept
{
  GetRepositoryAutoUpdate().Start();
}

void
RepositoryAutoUpdateTerrainIdle() noexcept
{
  GetRepositoryAutoUpdate().TerrainIdle();
}

void
RepositoryAutoUpdateShutdown() noexcept
{
  GetRepositoryAutoUpdate().Shutdown();
}

#endif
