// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadGlue.hpp"

#ifdef HAVE_EDL

#include "TileStore.hpp"
#include "Operation/ProgressListener.hpp"
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
DownloadGlue::DeliverNotification(DownloadNotification notification) noexcept
{
  NotifyListeners(notification);
}

void
DownloadGlue::NotifyListeners(const DownloadNotification &notification) noexcept
{
  listeners.ForEach([&notification](DownloadListener *listener) {
    listener->OnDownloadFinished(notification);
  });
}

void
DownloadGlue::StartOverlayDownload(BrokenDateTime forecast_time,
                                    unsigned isobar) noexcept
{
  if (shutting_down || task.IsRunning())
    return;

  pending_job = Job::OVERLAY;
  completion_error = {};
  completed_path.reset();
  overlay_forecast_time = forecast_time;
  overlay_isobar = isobar;

  task.Start(RunOverlayDownload(), BIND_THIS_METHOD(OnCompletion));
}

void
DownloadGlue::StartPrecacheDay(BrokenDateTime day) noexcept
{
  if (shutting_down || task.IsRunning())
    return;

  pending_job = Job::PRECACHE_DAY;
  completion_error = {};
  completed_precache_count = 0;
  completed_path.reset();

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

  DownloadNotification notification;
  if (job == Job::OVERLAY)
    notification.job = DownloadJob::OVERLAY;
  else if (job == Job::PRECACHE_DAY)
    notification.job = DownloadJob::PRECACHE_DAY;
  else
    return;

  if (completion_error) {
    notification.outcome = DownloadOutcome::ERROR;
    notification.error = completion_error;
  } else {
    notification.outcome = DownloadOutcome::SUCCESS;
    if (job == Job::OVERLAY)
      notification.overlay_path = std::move(completed_path);
    else
      notification.precache_count = completed_precache_count;
  }

  completed_path.reset();
  completed_precache_count = 0;
  completion_error = {};

  NotifyListeners(notification);
}

} // namespace EDL

#endif /* HAVE_EDL */
