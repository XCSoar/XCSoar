// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadGlue.hpp"

#ifdef HAVE_EDL

#include "TileStore.hpp"
#include "Weather/BackgroundDownloadProgress.hpp"
#include "Language/Language.hpp"
#include "Operation/ProgressListener.hpp"
#include "LogFile.hpp"
#include "lib/curl/Global.hxx"
#include "util/Macros.hpp"
#include "util/StaticString.hxx"

namespace EDL {

namespace {

StaticString<128>
FormatOverlayProgressText(BrokenDateTime forecast_time,
                          unsigned isobar) noexcept
{
  StaticString<128> text;
  text.Format(_("Loading EDL overlay %u hPa %s..."),
              isobar / 100, FormatForecastUtcLog(forecast_time).c_str());
  return text;
}

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
  overlay_restart_pending = false;
  completion_error = {};
  completed_path.reset();
  task.BeginShutdown();
  complete_notify.ClearNotification();
  BackgroundDownloadProgress::Get().ForceHide();
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

bool
DownloadGlue::StartOverlayDownload(BrokenDateTime forecast_time,
                                    unsigned isobar) noexcept
{
  if (shutting_down)
    return false;

  overlay_forecast_time = forecast_time;
  overlay_isobar = isobar;
  pending_job = Job::OVERLAY;

  if (task.IsRunning()) {
    overlay_restart_pending = true;
    LogFmt("edl: overlay download queued {} hPa {}",
           isobar / 100, FormatForecastUtcLog(forecast_time).c_str());
    return false;
  }

  overlay_restart_pending = false;
  completion_error = {};
  completed_path.reset();

  LogFmt("edl: starting overlay download {} hPa {}",
         isobar / 100, FormatForecastUtcLog(forecast_time).c_str());

  BackgroundDownloadProgress::Get().Begin(
    FormatOverlayProgressText(forecast_time, isobar).c_str());

  task.Start(RunOverlayDownload(), BIND_THIS_METHOD(OnCompletion));
  return true;
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

  StaticString<128> text;
  text.Format(_("Caching EDL forecast %s..."),
              FormatForecastDayLog(day).c_str());
  BackgroundDownloadProgress::Get().Begin(text.c_str());

  task.Start(RunPrecacheDay(day), BIND_THIS_METHOD(OnCompletion));
}

Co::InvokeTask
DownloadGlue::RunOverlayDownload()
{
  auto &progress = BackgroundDownloadProgress::Get();
  const TileRequest request(overlay_forecast_time, overlay_isobar);
  completed_path = co_await request.EnsureDownloaded(curl, progress);
}

Co::InvokeTask
DownloadGlue::RunPrecacheDay(BrokenDateTime day)
{
  completed_precache_count =
    co_await EnsureDayDownloaded(day, curl, BackgroundDownloadProgress::Get());
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
    notification.outcome = DownloadOutcome::DOWNLOAD_ERROR;
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

  if (overlay_restart_pending && !shutting_down) {
    overlay_restart_pending = false;
    pending_job = Job::OVERLAY;
    completion_error = {};
    completed_path.reset();
    LogFmt("edl: restarting queued overlay download {} hPa {}",
           overlay_isobar / 100,
           FormatForecastUtcLog(overlay_forecast_time).c_str());
    task.Start(RunOverlayDownload(), BIND_THIS_METHOD(OnCompletion));
    return;
  }

  if (!task.IsRunning())
    BackgroundDownloadProgress::Get().End();
}

} // namespace EDL

#endif /* HAVE_EDL */
