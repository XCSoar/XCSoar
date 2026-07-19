// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadGlue.hpp"

#ifdef HAVE_DOWNLOAD_MANAGER

#include "Configured.hpp"
#include "Weather/BackgroundDownloadProgress.hpp"
#include "Components.hpp"
#include "DataGlobals.hpp"
#include "Language/Language.hpp"
#include "NetComponents.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/FileType.hpp"
#include "Repository/Glue.hpp"
#include "LogFile.hpp"
#include "util/StringCompare.hxx"
#include "net/http/DownloadManager.hpp"
#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "util/StaticString.hxx"
#include "Weather/MapOverlay/ControlsWidget.hpp"
#include "Weather/Settings.hpp"

#include <string_view>

using std::string_view_literals::operator""sv;

namespace {

bool
IsTrackedDownloadPath(Path path_relative) noexcept
{
  const auto name = path_relative.GetBase();
  if (name == nullptr || name.empty())
    return false;

  if (StringIsEqual(name.c_str(), "repository"))
    return true;

  if (IsUserRepositoryFile(name.c_str()))
    return true;

  const auto configured = GetConfiguredRaspFileName();
  if (configured == nullptr)
    return false;

  return StringIsEqual(name.c_str(), configured.c_str());
}

} // namespace

RaspDownloadGlue *
GetRaspDownloadGlue() noexcept
{
  if (net_components == nullptr)
    return nullptr;
  return net_components->rasp_download.get();
}

RaspDownloadGlue::~RaspDownloadGlue() noexcept
{
  BeginShutdown();
}

void
RaspDownloadGlue::Initialise() noexcept
{
  if (listener_registered || !Net::DownloadManager::IsAvailable())
    return;

  Net::DownloadManager::AddListener(listener);
  listener_registered = true;

  /* Pick up downloads already queued (e.g. before glue registered). */
  Net::DownloadManager::Enumerate(listener);
}

void
RaspDownloadGlue::BeginShutdown() noexcept
{
  if (listener_registered) {
    Net::DownloadManager::RemoveListener(listener);
    listener_registered = false;
  }

  StopProgress();
  BackgroundDownloadProgress::Get().ForceHide();
  pending_show_progress.store(false);
  pending_completion.store(PendingCompletion::NONE);
  download_notify.ClearNotification();
}

void
RaspDownloadGlue::RequestUpdateIfOutOfDate() noexcept
{
  QueueConfiguredRaspUpdate(false, false, "RASP repository update failed");
}

void
RaspDownloadGlue::RequestConfiguredRaspUpdate() noexcept
{
  QueueConfiguredRaspUpdate(true, true, "RASP repository download failed");
}

bool
RaspDownloadGlue::QueueConfiguredRaspUpdate(bool force_repository,
                                            bool force_rasp_download,
                                            const char *error_message) noexcept
{
  if (!Net::DownloadManager::IsAvailable())
    return false;

  try {
    EnqueueRepositoryDownload(force_repository, true, true);

    FileRepository repository;
    LoadAllRepositories(repository);
    if (force_rasp_download)
      EnqueueConfiguredRaspDownload(repository);
    else
      EnqueueConfiguredRaspUpdate(repository);

    return true;
  } catch (...) {
    LogError(std::current_exception(), error_message);
    return false;
  }
}

bool
RaspDownloadGlue::IsRaspDownload(Path path_relative) noexcept
{
  const auto name = path_relative.GetBase();
  if (name == nullptr || name.empty())
    return false;

  const auto configured = GetConfiguredRaspFileName();
  if (configured == nullptr)
    return false;

  return StringIsEqual(name.c_str(), configured.c_str());
}

bool
RaspDownloadGlue::IsTrackedDownload(Path path_relative) noexcept
{
  return IsTrackedDownloadPath(path_relative);
}

void
RaspDownloadGlue::EnsureProgressActive() noexcept
{
  if (progress_active)
    return;

  progress_active = true;
  BackgroundDownloadProgress::Get().Begin(_("Downloading RASP..."));
  BackgroundDownloadProgress::Get().SetProgressRange(100);
  progress_timer.Schedule(std::chrono::milliseconds(200));
  PollDownloadProgress();
}

void
RaspDownloadGlue::StopProgress() noexcept
{
  if (!progress_active)
    return;

  progress_active = false;
  progress_timer.Cancel();
  BackgroundDownloadProgress::Get().End();
}

void
RaspDownloadGlue::ReloadConfiguredRasp() noexcept
{
  auto rasp = LoadConfiguredRasp(false);
  if (rasp == nullptr)
    return;

  DataGlobals::SetRasp(std::move(rasp));
}

void
RaspDownloadGlue::PollDownloadProgress() noexcept
{
  if (!progress_active)
    return;

  struct Poller final : public Net::DownloadListener {
    bool found = false;
    int64_t size = -1;
    int64_t position = -1;
    StaticString<64> label;

    void OnDownloadAdded(Path path, int64_t s, int64_t p) noexcept override {
      if (!IsTrackedDownloadPath(path) || found)
        return;

      found = true;
      size = s;
      position = p;

      /* Always assign: StaticString is uninitialized by default, and
         PollDownloadProgress formats label once found is true. */
      const auto base = path.GetBase();
      if (base == nullptr || base.empty())
        label = _("Updating");
      else if (StringIsEqual(base.c_str(), "repository") ||
               IsUserRepositoryFile(base.c_str()))
        label = _("Updating repository");
      else
        label = base.c_str();
    }

    void OnDownloadComplete(Path) noexcept override {}
    void OnDownloadError(Path, std::exception_ptr) noexcept override {}
  } poller;

  Net::DownloadManager::Enumerate(poller);

  if (!poller.found) {
    FileRepository repository;
    LoadAllRepositories(repository);
    if (!IsConfiguredRaspOutOfDate(repository))
      StopProgress();
    return;
  }

  StaticString<128> text;
  unsigned percent = 0;
  if (poller.size > 0 && poller.position >= 0) {
    percent = unsigned(poller.position * 100 / poller.size);
    if (percent > 100)
      percent = 100;
    text.Format(_("%s (%u%%)..."), poller.label.c_str(), percent);
  } else
    text.Format(_("%s..."), poller.label.c_str());

  BackgroundDownloadProgress::Get().SetText(text.c_str());
  BackgroundDownloadProgress::Get().SetProgressRange(100);
  if (poller.size > 0 && poller.position >= 0)
    BackgroundDownloadProgress::Get().SetProgressPosition(percent);
}

void
RaspDownloadGlue::OnDownloadNotify() noexcept
{
  if (pending_show_progress.exchange(false)) {
    if (!progress_active)
      EnsureProgressActive();
  }

  switch (pending_completion.exchange(PendingCompletion::NONE)) {
  case PendingCompletion::REPOSITORY:
    RequestUpdateIfOutOfDate();
    break;

  case PendingCompletion::RASP_RELOAD:
    StopProgress();
    ReloadConfiguredRasp();
    ActionInterface::ScheduleSendUIState();
    /* Clear stale [no data] on the weather cursor bar once the new
       file's time slots are available. */
    WeatherMapOverlay::RefreshControlsLabels();
    break;

  case PendingCompletion::RASP_ERROR:
    StopProgress();
    return;

  case PendingCompletion::NONE:
    break;
  }

  PollDownloadProgress();
}

void
RaspDownloadGlue::Listener::OnDownloadAdded(Path path_relative,
                                            int64_t size,
                                            int64_t position) noexcept
{
  (void)size;
  (void)position;

  if (!IsTrackedDownload(path_relative))
    return;

  owner.pending_show_progress.store(true);
  owner.download_notify.SendNotification();
}

void
RaspDownloadGlue::Listener::OnDownloadComplete(Path path_relative) noexcept
{
  const auto name = path_relative.GetBase();
  if (name == nullptr || name.empty())
    return;

  if (name.c_str() == "repository"sv || IsUserRepositoryFile(name.c_str())) {
    owner.pending_completion.store(PendingCompletion::REPOSITORY);
    owner.download_notify.SendNotification();
    return;
  }

  if (!IsRaspDownload(path_relative))
    return;

  owner.pending_completion.store(PendingCompletion::RASP_RELOAD);
  owner.download_notify.SendNotification();
}

void
RaspDownloadGlue::Listener::OnDownloadError(
  Path path_relative, [[maybe_unused]] std::exception_ptr error) noexcept
{
  if (!IsRaspDownload(path_relative))
    return;

  owner.pending_completion.store(PendingCompletion::RASP_ERROR);
  owner.download_notify.SendNotification();
}

void
RequestConfiguredRaspUpdateIfOutOfDate() noexcept
{
  if (!CommonInterface::GetComputerSettings().weather.rasp.auto_update)
    return;

  if (RaspDownloadGlue *glue = GetRaspDownloadGlue())
    glue->RequestUpdateIfOutOfDate();
}

void
RequestConfiguredRaspUpdate() noexcept
{
  if (RaspDownloadGlue *glue = GetRaspDownloadGlue())
    glue->RequestConfiguredRaspUpdate();
}

#endif /* HAVE_DOWNLOAD_MANAGER */
