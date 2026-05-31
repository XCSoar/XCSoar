// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Glue.hpp"

#include "StateController.hpp"
#include "NMEA/MoreData.hpp"

#ifdef HAVE_EDL

#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "Message.hpp"
#include "util/StaticString.hxx"

namespace EDL {

static Glue *registered_glue = nullptr;

void
RequestOverlayRefresh() noexcept
{
  if (registered_glue != nullptr)
    registered_glue->RequestOverlayRefresh();
}

void
RequestPrecacheDay(BrokenDateTime day) noexcept
{
  if (registered_glue != nullptr)
    registered_glue->RequestPrecacheDay(day);
}

void
Glue::AttachDownloadGlue(DownloadGlue &download) noexcept
{
  download_glue = &download;
  download.AddListener(*this);
  registered_glue = this;
}

void
Glue::DetachDownloadGlue() noexcept
{
  if (download_glue != nullptr) {
    download_glue->RemoveListener(*this);
    download_glue = nullptr;
  }

  if (registered_glue == this)
    registered_glue = nullptr;
}

void
Glue::RequestOverlayRefresh() noexcept
{
  if (download_glue == nullptr)
    return;

  if (!OverlayEnabled()) {
    download_glue->DeliverNotification(DownloadNotification{
      .job = DownloadJob::OVERLAY,
      .outcome = DownloadOutcome::SUCCESS,
    });
    return;
  }

  if (TryApplyOverlayFromCache()) {
    ActionInterface::ScheduleSendUIState();
    download_glue->DeliverNotification(DownloadNotification{
      .job = DownloadJob::OVERLAY,
      .outcome = DownloadOutcome::SUCCESS,
      .overlay_from_cache = true,
    });
    return;
  }

  SetLoadingStatus();
  download_glue->StartOverlayDownload(GetForecastTime(), GetIsobar());
}

void
Glue::RequestPrecacheDay(BrokenDateTime day) noexcept
{
  if (download_glue == nullptr || !OverlayEnabled())
    return;

  SetLoadingStatus();
  download_glue->StartPrecacheDay(day);
}

void
Glue::OnDownloadFinished(const DownloadNotification &notification) noexcept
{
  if (notification.overlay_from_cache)
    return;

  switch (notification.job) {
  case DownloadJob::OVERLAY:
    if (notification.outcome == DownloadOutcome::ERROR) {
      LogError(notification.error, "EDL overlay download failed");
      SetErrorStatus();
    } else if (notification.overlay_path) {
      if (ShouldMaintainOverlay())
        ApplyOverlay(*notification.overlay_path);
      else
        SetIdleStatus();
    } else {
      SetIdleStatus();
    }

    ActionInterface::ScheduleSendUIState();
    break;

  case DownloadJob::PRECACHE_DAY:
    if (notification.outcome == DownloadOutcome::ERROR) {
      LogError(notification.error, "EDL precache failed");
      SetErrorStatus();
    } else {
      SetIdleStatus();

      StaticString<64> message;
      message.Format(_("Cached %u files for the selected UTC day."),
                     notification.precache_count);
      Message::AddMessage(message);
    }
    break;
  }
}

} // namespace EDL

#endif /* HAVE_EDL */

namespace EDL {

void
Glue::OnGPSUpdate(const MoreData &basic)
{
  ProcessGpsUpdate(basic.date_time_utc);
}

} // namespace EDL
