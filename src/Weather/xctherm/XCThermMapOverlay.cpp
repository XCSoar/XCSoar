// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermMapOverlay.hpp"
#include "XCThermAPI.hpp"
#include "XCThermCatalog.hpp"
#include "XCThermForecastTime.hpp"
#include "XCThermDownloadGlue.hpp"
#include "XCThermDownloadJob.hpp"
#include "XCThermGeoJSONOverlay.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "UIGlobals.hpp"
#include "Weather/Settings.hpp"
#include "net/http/Init.hpp"

#include <mutex>

namespace XCTherm {

namespace {

std::string auto_fetch_attempted_param;

} // namespace

void
ApplyForecastToMap(const std::string &geojson, const char *label,
                   const char *parameter, const unsigned forecast_utc) noexcept
{
  auto forecast = XCThermGeoJSON::Parse(geojson, true);
  if (forecast.IsEmpty()) {
    LogFmt("xctherm: parse failed for {}", label);
    return;
  }

  forecast.layer_name = label;
  ApplyForecastLayerToMap(std::move(forecast), label, parameter, forecast_utc);
}

void
ApplyForecastLayerToMap(XCThermGeoJSON::ForecastLayer &&forecast,
                        const char *label, const char *parameter,
                        const unsigned forecast_utc) noexcept
{
  if (forecast.IsEmpty())
    return;

  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

#ifdef ENABLE_OPENGL
  auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
  overlay->SetForecast(std::move(forecast), label, parameter, forecast_utc);
  map->SetOverlay(std::move(overlay));
#else
  (void)label;
  (void)parameter;
  (void)forecast_utc;
#endif
}

void
ClearMapOverlay() noexcept
{
#ifdef ENABLE_OPENGL
  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return;

  if (dynamic_cast<const XCThermGeoJSONOverlay *>(map->GetOverlay()) ==
      nullptr)
    return;

  map->SetOverlay(nullptr);
#endif
}

void
ApplyCursorOverlayFromSession() noexcept
{
  const auto &session =
    CommonInterface::GetUIState().weather.xctherm;
  if (!session.cursor_initialized) {
    RestoreActiveLayerOverlay();
    return;
  }

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = GetRegion(settings.model);
  if (session.cursor_layer >= region.layer_count)
    return;

  const auto &layer = region.layers[session.cursor_layer];
  if (!XCThermAPI::Instance().IsLayerCached(layer.api_parameter,
                                             session.cursor_forecast_utc_hour)) {
    ClearMapOverlay();
    return;
  }

  ApplyCachedLayerOverlay(session.cursor_layer,
                          session.cursor_forecast_utc_hour);
}

bool
EnterDedicatedPage() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.xctherm;
  if (state.dedicated_page_entered)
    return false;

  state.dedicated_page_entered = true;
  return true;
}

void
LeaveDedicatedPage() noexcept
{
  CommonInterface::SetUIState().weather.xctherm.dedicated_page_entered = false;
}

void
SuspendDedicatedPageForPan() noexcept
{
  auto &state = CommonInterface::SetUIState().weather.xctherm;
  state.dedicated_page_suspended_for_pan = state.dedicated_page_entered;
}

void
ResumeDedicatedPageAfterPan() noexcept
{
  CommonInterface::SetUIState().weather.xctherm
    .dedicated_page_suspended_for_pan = false;
}

bool
IsDedicatedPageSuspendedForPan() noexcept
{
  return CommonInterface::GetUIState().weather.xctherm
    .dedicated_page_suspended_for_pan;
}

StaticString<64>
GetPanOverlayLabel() noexcept
{
  StaticString<64> label;
#ifdef ENABLE_OPENGL
  const auto *map = UIGlobals::GetMap();
  if (map != nullptr) {
    const auto *xctherm =
      dynamic_cast<const XCThermGeoJSONOverlay *>(map->GetOverlay());
    if (xctherm != nullptr) {
      const char *layer = xctherm->GetLabel();
      if (layer != nullptr && *layer != '\0') {
        label.Format(_("XCTherm %s"), layer);
        return label;
      }
    }
  }
#endif

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const int li = FindActiveLayerIndex(settings);
  if (li >= 0) {
    label.Format(_("XCTherm %s"),
                 gettext(GetRegion(settings.model)
                           .layers[unsigned(li)].short_label));
  } else
    label = _("XCTherm");

  return label;
}

bool
ApplyCachedLayerOverlay(unsigned layer_index, unsigned utc_hour) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = GetRegion(settings.model);
  if (layer_index >= region.layer_count)
    return false;

  auto &api = XCThermAPI::Instance();
  const auto &layer = region.layers[layer_index];
  const std::string &cached =
    api.GetCachedGeoJSON(layer.api_parameter, utc_hour);
  if (cached.empty()) {
    LogFmt("xctherm: cache miss {}@{}h", layer.api_parameter, utc_hour);
    return false;
  }

  ApplyForecastToMap(cached, gettext(layer.short_label),
                       layer.api_parameter, utc_hour);
  return true;
}

bool
ApplyCachedLayerOverlayAuto(unsigned layer_index) noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = GetRegion(settings.model);
  if (layer_index >= region.layer_count)
    return false;

  const auto &layer = region.layers[layer_index];
  const auto hours =
    XCThermAPI::Instance().GetCachedHours(layer.api_parameter);
  if (hours.empty())
    return false;

  const auto utc = GetUtcTimeParts();
  const int idx = PickAutoTimeIndex(hours, utc.hour, utc.minute);
  const unsigned hour = idx >= 0 ? hours[unsigned(idx)] : hours[0];
  return ApplyCachedLayerOverlay(layer_index, hour);
}

void
RestoreActiveLayerOverlay() noexcept
{
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const int layer_index = ResolveDisplayLayerIndex(settings);
  if (layer_index < 0)
    return;

  if (!ApplyCachedLayerOverlayAuto(unsigned(layer_index)))
    ClearMapOverlay();
}

void
ApplyJobPreviewToMap(const std::shared_ptr<XCThermDownloadJob> &job) noexcept
{
  if (job == nullptr || job->cancel.load() ||
      job->error_eptr || job->succeeded_or_cached.load() == 0)
    return;

  std::lock_guard lock{job->result_mutex};
  if (!job->first_forecast.IsEmpty()) {
    const unsigned shown_utc = job->has_first_forecast_utc
      ? job->first_forecast_utc
      : (job->current_utc + 23) % 24;
    ApplyForecastLayerToMap(std::move(job->first_forecast),
                            gettext(job->target_label.c_str()),
                            job->param.c_str(), shown_utc);
  } else {
    RestoreActiveLayerOverlay();
  }
}

std::shared_ptr<XCThermDownloadJob>
StartSpanDownload(
  const XCThermSettings &settings, unsigned layer_index,
  std::function<void(std::shared_ptr<XCThermDownloadJob>)> on_finished)
{
  if (settings.download_span_hours == 0 || !settings.credentials.IsDefined())
    return nullptr;

  auto *glue = GetXCThermDownloadGlue();
  if (glue == nullptr || Net::curl == nullptr || glue->IsRunning())
    return nullptr;

  auto job = MakeXCThermSpanJob(settings, layer_index, GetUtcTimeParts().hour);
  if (job == nullptr)
    return nullptr;

  auto shared = job;
  glue->Start(std::move(job), std::move(on_finished));
  return shared;
}

void
RequestBackgroundIndexFetch(std::function<void()> on_ready)
{
  auto *glue = GetXCThermDownloadGlue();
  if (glue == nullptr)
    return;

  if (XCThermAPI::Instance().IsIndexLoaded()) {
    if (on_ready)
      on_ready();
    return;
  }

  glue->StartIndexFetch(std::move(on_ready));
}

void
MaybeFetchActiveLayerSpan(
  std::function<void(std::shared_ptr<XCThermDownloadJob>)> on_finished)
{
  if (!XCThermAPI::Instance().IsIndexLoaded())
    return;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const int active = FindActiveLayerIndex(settings);
  if (active < 0)
    return;

  const auto &region = GetRegion(settings.model);
  const std::string &param = region.layers[unsigned(active)].api_parameter;
  if (!XCThermAPI::Instance().GetCachedHours(param).empty())
    return;

  if (auto_fetch_attempted_param == param)
    return;

  auto_fetch_attempted_param = param;
  LogFmt("xctherm: auto-fetching active layer {}", param);

  if (!StartSpanDownload(settings, unsigned(active),
                         [finished = std::move(on_finished)](
                           std::shared_ptr<XCThermDownloadJob> done) mutable {
                           ApplyJobPreviewToMap(done);
                           if (finished)
                             finished(std::move(done));
                         }))
    auto_fetch_attempted_param.clear();
}

void
ResetAutoFetchAttempt() noexcept
{
  auto_fetch_attempted_param.clear();
}

void
ActivatePageOverlay() noexcept
{
  auto &session = CommonInterface::SetUIState().weather.xctherm;
  if (session.altitude_manual_override || session.time_manual_override)
    session.cursor_initialized = true;

  EnterDedicatedPage();

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  XCThermAPI::Instance().PrepareSession(settings);
  ApplyCursorOverlayFromSession();

  RequestBackgroundIndexFetch([]{
    ApplyCursorOverlayFromSession();
    MaybeFetchActiveLayerSpan(nullptr);
  });
}

} // namespace XCTherm
