// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadGlue.hpp"

#ifdef HAVE_EDL

#include "StateController.hpp"
#include "TileStore.hpp"
#include "ActionInterface.hpp"
#include "Operation/ProgressListener.hpp"
#include "Message.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"
#include "LogFile.hpp"
#include "lib/curl/Global.hxx"
#include "util/Macros.hpp"

namespace EDL {

namespace {

struct NullProgressListener final : ProgressListener {
  void SetProgressRange(unsigned) noexcept override {}
  void SetProgressPosition(unsigned) noexcept override {}
};

} // namespace

DownloadGlue::DownloadGlue(CurlGlobal &_curl) noexcept
  :curl(_curl),
   task(curl.GetEventLoop())
{
}

DownloadGlue::~DownloadGlue() noexcept
{
  BeginShutdown();
}

void
DownloadGlue::BeginShutdown() noexcept
{
  if (shutting_down)
    return;

  shutting_down = true;
  pending_job = Job::NONE;
  completion_error = {};
  completed_path.reset();
  task.BeginShutdown();
  complete_notify.ClearNotification();
}

void
DownloadGlue::AddListener(DownloadListener &listener) noexcept
{
  listeners.Add(&listener);
}

void
DownloadGlue::RemoveListener(DownloadListener &listener) noexcept
{
  listeners.Remove(&listener);
}

void
DownloadGlue::NotifyListeners() noexcept
{
  listeners.ForEach([](DownloadListener *listener) {
    listener->OnDownloadFinished();
  });
}

void
DownloadGlue::RequestOverlayRefresh() noexcept
{
  if (shutting_down || task.IsRunning())
    return;

  if (!OverlayEnabled()) {
    NotifyListeners();
    return;
  }

  if (TryApplyOverlayFromCache()) {
    ActionInterface::SendUIState(true);
    NotifyListeners();
    return;
  }

  pending_job = Job::OVERLAY;
  completion_error = {};
  completed_path.reset();
  overlay_forecast_time = GetForecastTime();
  overlay_isobar = GetIsobar();

  SetLoadingStatus();
  task.Start(RunOverlayDownload(), BIND_THIS_METHOD(OnCompletion));
}

void
DownloadGlue::RequestPrecacheDay(BrokenDateTime day) noexcept
{
  if (shutting_down || task.IsRunning() || !OverlayEnabled())
    return;

  pending_job = Job::PRECACHE_DAY;
  completion_error = {};
  completed_precache_count = 0;
  completed_path.reset();

  SetLoadingStatus();
  task.Start(RunPrecacheDay(day), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
DownloadGlue::RunOverlayDownload()
{
  NullProgressListener progress;
  const TileRequest request(overlay_forecast_time, overlay_isobar);
  completed_path = co_await request.EnsureDownloaded(curl, progress);
}

Co::InvokeTask
DownloadGlue::RunPrecacheDay(BrokenDateTime day)
{
  NullProgressListener progress;
  completed_precache_count =
    co_await EnsureDayDownloaded(day, curl, progress);
}

void
DownloadGlue::OnCompletion(std::exception_ptr error) noexcept
{
  completion_error = std::move(error);
  complete_notify.SendNotification();
}

void
DownloadGlue::OnCompleteNotify() noexcept
{
  if (shutting_down)
    return;

  const Job job = pending_job;
  pending_job = Job::NONE;

  if (job == Job::OVERLAY) {
    if (completion_error) {
      LogError(completion_error, "EDL overlay download failed");
      SetErrorStatus();
    } else if (completed_path) {
      if (ShouldMaintainOverlay())
        ApplyOverlay(*completed_path);
      else
        SetIdleStatus();
    } else {
      SetIdleStatus();
    }

    completed_path.reset();
    ActionInterface::SendUIState(true);
  } else if (job == Job::PRECACHE_DAY) {
    if (completion_error) {
      LogError(completion_error, "EDL precache failed");
      SetErrorStatus();
    } else {
      SetIdleStatus();

      StaticString<64> message;
      message.Format(_("Cached %u files for the selected UTC day."),
                     completed_precache_count);
      Message::AddMessage(message);
    }

    completed_precache_count = 0;
  }

  completion_error = {};
  NotifyListeners();
}

} // namespace EDL

#endif /* HAVE_EDL */
