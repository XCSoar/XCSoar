// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermMapOverlay.hpp"
#include "PageSettings.hpp"
#include "XCThermAPI.hpp"
#include "XCThermCatalog.hpp"
#include "XCThermForecastTime.hpp"
#include "XCThermDownloadGlue.hpp"
#include "XCThermDownloadJob.hpp"
#include "XCThermGeoJSONOverlay.hpp"
#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "LogFile.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "UIGlobals.hpp"
#include "Weather/Settings.hpp"
#include "net/http/Init.hpp"

#include <mutex>
#include <string_view>

namespace XCTherm {

namespace {

std::string auto_fetch_attempted_param;
bool page_overlay_suspended_for_pan = false;

const XCThermGeoJSONOverlay *
GetXCThermMapOverlay() noexcept
{
#ifdef ENABLE_OPENGL
  const auto *map = UIGlobals::GetMap();
  if (map == nullptr || map->GetOverlay() == nullptr)
    return nullptr;

  return dynamic_cast<const XCThermGeoJSONOverlay *>(map->GetOverlay());
#else
  return nullptr;
#endif
}

bool
MapOverlayShowsCachedLayer(unsigned layer_index, unsigned utc_hour) noexcept
{
  const auto *overlay = GetXCThermMapOverlay();
  if (overlay == nullptr || !overlay->HasData())
    return false;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;
  const auto &region = GetRegion(settings.model);
  if (layer_index >= region.layer_count)
    return false;

  const auto &layer = region.layers[layer_index];
  return overlay->GetParameter() == layer.api_parameter &&
    overlay->GetForecastUtc() == utc_hour;
}

void
RefreshMapAfterOverlayChange() noexcept
{
  if (auto *map = UIGlobals::GetMap())
    map->PartialRedraw();
  ActionInterface::SendUIState(true);
}

} // namespace

const XCThermGeoJSONOverlay *
GetMapOverlay() noexcept
{
  return GetXCThermMapOverlay();
}

bool
FindLayerByApiParameter(std::string_view api_parameter,
                        unsigned &altitude_m, bool &is_agl) noexcept
{
  for (unsigned r = 0; r < unsigned(Region::COUNT); ++r) {
    const auto &def = GetRegion(Region(r));
    for (unsigned i = 0; i < def.layer_count; ++i) {
      const auto &layer = def.layers[i];
      if (api_parameter == layer.api_parameter) {
        altitude_m = layer.altitude_m;
        is_agl = layer.is_agl;
        return true;
      }
    }
  }

  return false;
}

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

  auto overlay = std::make_unique<XCThermGeoJSONOverlay>();
  overlay->SetForecast(std::move(forecast), label, parameter, forecast_utc);
  map->SetOverlay(std::move(overlay));
  RefreshMapAfterOverlayChange();
}

void
ClearMapOverlay() noexcept
{
  auto *map = UIGlobals::GetMap();
  if (map != nullptr) {
    map->SetOverlay(nullptr);
    RefreshMapAfterOverlayChange();
  }
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

  if (MapOverlayShowsCachedLayer(layer_index, utc_hour))
    return true;

  ApplyForecastToMap(cached, layer.short_label, layer.api_parameter, utc_hour);
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

  ApplyCachedLayerOverlayAuto(unsigned(layer_index));
}

void
ApplyJobPreviewToMap(const std::shared_ptr<XCThermDownloadJob> &job) noexcept
{
  if (job == nullptr || job->cancel.load() ||
      job->error_eptr || job->succeeded_or_cached.load() == 0)
    return;

  std::lock_guard lock{job->result_mutex};
  if (!job->first_forecast.IsEmpty()) {
    const unsigned shown_utc = (job->current_utc + 23) % 24;
    ApplyForecastLayerToMap(std::move(job->first_forecast),
                            job->target_label.c_str(),
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
SuspendPageOverlayForPan() noexcept
{
  page_overlay_suspended_for_pan = true;
}

void
ResumePageOverlayAfterPan() noexcept
{
  page_overlay_suspended_for_pan = false;
}

bool
IsPageOverlaySuspendedForPan() noexcept
{
  return page_overlay_suspended_for_pan;
}

bool
HasMapOverlay() noexcept
{
  return GetXCThermMapOverlay() != nullptr;
}

bool
ShouldSuspendPageOverlayForPan(const PageLayout &layout) noexcept
{
  if (!layout.IsMapMain())
    return false;

  if (layout.UsesXcthermOverlay() ||
      layout.bottom == PageLayout::Bottom::XCTHERM)
    return true;

  return HasMapOverlay();
}

void
ApplyPageOverlayForLayout(const PageLayout &page_layout) noexcept
{
  if (IsPageOverlaySuspendedForPan() && HasMapOverlay()) {
    RefreshMapAfterOverlayChange();
    return;
  }

  auto &api = XCThermAPI::Instance();
  const auto &settings =
    CommonInterface::GetComputerSettings().weather.xctherm;

  /* Bottom cursor bar runs Restore / OnIndexLoaded when cache exists. */
  const bool cursor_bar_handles_overlay =
    page_layout.bottom == PageLayout::Bottom::XCTHERM &&
    api.HasAnyCache();

  if (!cursor_bar_handles_overlay && !HasMapOverlay())
    ResetAutoFetchAttempt();

  api.PrepareSession(settings);

  if (!cursor_bar_handles_overlay)
    RestoreActiveLayerOverlay();

  RefreshMapAfterOverlayChange();

  if (api.IsIndexLoaded()) {
    if (!cursor_bar_handles_overlay)
      MaybeFetchActiveLayerSpan(nullptr);
    return;
  }

  if (cursor_bar_handles_overlay)
    return;

  RequestBackgroundIndexFetch([]{
    RestoreActiveLayerOverlay();
    MaybeFetchActiveLayerSpan(nullptr);
    RefreshMapAfterOverlayChange();
  });
}

} // namespace XCTherm
