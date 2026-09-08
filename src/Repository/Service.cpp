// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Service.hpp"
#include "Freshness.hpp"
#include "Glue.hpp"
#include "DataFilePath.hpp"
#include "system/FileUtil.hpp"

#include <algorithm>
#include <chrono>

namespace Repository {

static bool
IsMainRepositoryPath(Path path_relative) noexcept
{
  const Path name = path_relative.GetBase();
  return name != nullptr && name == Path("repository");
}

static bool
IsMainRepositoryFresh() noexcept
{
  const auto modified =
    File::GetLastModification(ResolveRepositoryDataPath("repository"));
  if (modified == std::chrono::system_clock::time_point{})
    return false;

  return !IsRefreshDue(modified, std::chrono::system_clock::now());
}

Service::Service()
  :has_snapshot(LoadMainRepository(repository))
{
  Net::DownloadManager::AddListener(*this);
}

Service::~Service() noexcept
{
  BeginShutdown();
}

bool
Service::HasFreshSnapshot() const noexcept
{
  return has_snapshot && IsMainRepositoryFresh();
}

void
Service::AddListener(Listener &listener)
{
  if (std::find(listeners.begin(), listeners.end(), &listener) ==
      listeners.end())
    listeners.push_back(&listener);
}

void
Service::RemoveListener(Listener &listener) noexcept
{
  const auto i = std::find(listeners.begin(), listeners.end(), &listener);
  if (i != listeners.end())
    listeners.erase(i);
}

RefreshResult
Service::Refresh(bool force) noexcept
{
  if (shutting_down || !Net::DownloadManager::IsAvailable())
    return RefreshResult::UNAVAILABLE;

  {
    const std::lock_guard lock{mutex};
    if (refreshing)
      return RefreshResult::IN_PROGRESS;
    if (!force && IsMainRepositoryFresh())
      return RefreshResult::FRESH;
    refreshing = true;
  }

  try {
    EnqueueRepositoryDownload(true, true, false);
  } catch (...) {
    const std::lock_guard lock{mutex};
    refreshing = false;
    return RefreshResult::UNAVAILABLE;
  }

  return RefreshResult::STARTED;
}

void
Service::OnDownloadComplete(Path path_relative) noexcept
{
  if (!IsMainRepositoryPath(path_relative))
    return;

  {
    const std::lock_guard lock{mutex};
    completion_pending = true;
    completion_success = true;
  }
  completion_notify.SendNotification();
}

void
Service::OnDownloadError(Path path_relative,
                         [[maybe_unused]] std::exception_ptr error) noexcept
{
  if (!IsMainRepositoryPath(path_relative))
    return;

  {
    const std::lock_guard lock{mutex};
    completion_pending = true;
    completion_success = false;
  }
  completion_notify.SendNotification();
}

void
Service::OnCompletionNotification() noexcept
{
  bool success;
  {
    const std::lock_guard lock{mutex};
    if (!completion_pending || shutting_down)
      return;
    completion_pending = false;
    success = completion_success;
    refreshing = false;
  }

  if (success) {
    FileRepository refreshed;
    success = LoadMainRepository(refreshed);
    if (success) {
      repository = std::move(refreshed);
      has_snapshot = true;
    } else
      /* Do not let an invalid download's new timestamp suppress retries. */
      File::Delete(ResolveRepositoryDataPath("repository"));
  }

  for (auto *listener : listeners)
    listener->OnRepositoryRefreshed(success);
}

void
Service::BeginShutdown() noexcept
{
  {
    const std::lock_guard lock{mutex};
    if (shutting_down)
      return;
    shutting_down = true;
    completion_pending = false;
    refreshing = false;
  }

  completion_notify.ClearNotification();
  Net::DownloadManager::RemoveListener(*this);
  listeners.clear();
}

} // namespace Repository
