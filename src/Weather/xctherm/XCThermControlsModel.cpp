// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermControlsModel.hpp"
#include "XCThermAPI.hpp"
#include "XCThermDownloadJob.hpp"
#include "XCThermForecastTime.hpp"
#include "XCThermMapOverlay.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "Weather/MapOverlay/CursorBarLabels.hpp"
#include "Weather/Settings.hpp"
#include "UIState.hpp"
#include "NMEA/Info.hpp"

#include <algorithm>

namespace XCTherm {

const RegionDef &
XCThermControlsModel::Region() const noexcept
{
  return GetRegion(Settings().model);
}

const Layer &
XCThermControlsModel::LayerAt(unsigned index) const noexcept
{
  return Region().layers[index];
}

const XCThermSettings &
XCThermControlsModel::Settings() const noexcept
{
  return CommonInterface::GetComputerSettings().weather.xctherm;
}

double
XCThermControlsModel::BestAltitude(const MoreData &basic) noexcept
{
  if (basic.gps_altitude_available)
    return basic.gps_altitude;
  if (basic.baro_altitude_available)
    return basic.baro_altitude;
  return -1.0;
}

void
XCThermControlsModel::ApplyAutoSwitchLayer(unsigned layer_index) noexcept
{
  if (!LayerUsableForAutoSwitch(layer_index))
    return;

  state.current_layer = layer_index;
  RefreshCachedHours();
  ApplyCurrentSelectionToMap();

  auto_switch.SyncCurrentLayerIndex(state.current_layer);
  SaveCursorSession();
  NotifyStateChanged();
}

void
XCThermControlsModel::ConfigureAutoSwitch() noexcept
{
  if (!auto_switch_configured) {
    auto_switch.SetLayerSwitchCallback([this](unsigned layer_index) {
      ApplyAutoSwitchLayer(layer_index);
    });
    auto_switch.SetTimeSwitchCallback([this](unsigned utc_hour) {
      if (!auto_switch.IsTimeAutoActive())
        return;

      state.current_time_index = utc_hour % 24;
      ApplyCurrentSelectionToMap();
      SaveCursorSession();
      NotifyStateChanged();
    });
    auto_switch_configured = true;
  }

  auto_switch.SetLoadedLayers(BuildAutoSwitchLayers());
  auto_switch.SyncCurrentLayerIndex(state.current_layer);
  auto_switch.SetLoadedTimes(state.available_hours);
  if (!state.available_hours.empty())
    auto_switch.SetCurrentTimePos(0);

  auto_switch.SetEnabled(Settings().auto_switch);
}

void
XCThermControlsModel::ApplyLayerToMap(unsigned layer_index,
                                      unsigned utc_hour) noexcept
{
  ApplyCachedLayerOverlay(layer_index, utc_hour);
}

unsigned
XCThermControlsModel::GetCurrentForecastHour() const noexcept
{
  return state.current_time_index % 24;
}

void
XCThermControlsModel::NotifyStateChanged() noexcept
{
  if (on_state_changed)
    on_state_changed();
}

void
XCThermControlsModel::Prepare(std::function<void()> state_changed) noexcept
{
  on_state_changed = std::move(state_changed);

  BootstrapSession();
  RequestBackgroundIndex([this]{
    OnIndexLoaded();
    NotifyStateChanged();
  });
  MaybeFetchActiveLayer([this](std::shared_ptr<XCThermDownloadJob> job) {
    OnDownloadFinished(job);
    NotifyStateChanged();
  });
}

void
XCThermControlsModel::OnShow() noexcept
{
  RefreshCachedHours();
  ApplyCurrentSelectionToMap();
}

void
XCThermControlsModel::OnHide() noexcept
{
  if (!IsDedicatedPageSuspendedForPan())
    ClearMapOverlay();
}

void
XCThermControlsModel::OnGPSUpdate(const MoreData &basic) noexcept
{
  auto_switch.SetEnabled(Settings().auto_switch);
  SyncActiveLayerFromSettings();

  if (!auto_switch.IsEnabled())
    return;

  double gps_alt = basic.gps_altitude_available ? basic.gps_altitude : -1.0;
  double baro_alt = basic.baro_altitude_available ? basic.baro_altitude : -1.0;

  unsigned utc_hour = 12;
  unsigned utc_minute = 0;
  if (basic.date_time_utc.IsPlausible()) {
    utc_hour = basic.date_time_utc.hour;
    utc_minute = basic.date_time_utc.minute;
  }

  auto_switch.Update(gps_alt, baro_alt, utc_hour, utc_minute, basic.time);
  SaveCursorSession();
}

void
XCThermControlsModel::LoadCursorSession() noexcept
{
  const auto &session = CommonInterface::GetUIState().weather.xctherm;
  if (!session.cursor_initialized)
    return;

  state.current_layer = session.cursor_layer;
  state.current_time_index = session.cursor_forecast_utc_hour;
  state.last_synced_active_layer = FindActiveLayerIndex(Settings());
  auto_switch.SetAltitudeManualOverride(session.altitude_manual_override);
  auto_switch.SetTimeManualOverride(session.time_manual_override);
}

void
XCThermControlsModel::SaveCursorSession() noexcept
{
  auto &session = CommonInterface::SetUIState().weather.xctherm;
  session.cursor_initialized = true;
  session.cursor_layer = state.current_layer;
  session.cursor_forecast_utc_hour = GetCurrentForecastHour();
  session.altitude_manual_override = auto_switch.IsAltitudeManualOverride();
  session.time_manual_override = auto_switch.IsTimeManualOverride();
}

void
XCThermControlsModel::BootstrapSession() noexcept
{
  auto &api = XCThermAPI::Instance();
  const auto &settings = Settings();

  api.PrepareSession(settings);

  ApplyIndexAvailability();
  if (!state.index_loaded)
    LogFmt("xctherm: index not loaded — fetching in background");

  LoadCursorSession();

  if (!CommonInterface::GetUIState().weather.xctherm.cursor_initialized) {
    const int def = FindLayerIndex(ToRegion(settings.model), 5000, false);
    if (def >= 0)
      state.current_layer = unsigned(def);

    SyncActiveLayerFromSettings();
    RefreshCachedHours();
    SelectBestTimeIndex();
    SaveCursorSession();
  } else {
    RefreshCachedHours();
    ConfigureAutoSwitch();
  }
}

void
XCThermControlsModel::ApplyIndexAvailability() noexcept
{
  auto &api = XCThermAPI::Instance();
  state.index_loaded = api.IsIndexLoaded();
  if (state.index_loaded) {
    const auto &params = api.GetAvailableParameters();
    for (unsigned i = 0; i < Region().layer_count; ++i) {
      state.layer_available[i] = false;
      for (const auto &p : params) {
        if (p.name == LayerAt(i).api_parameter) {
          state.layer_available[i] = true;
          break;
        }
      }
    }
    if (state.layer_available[state.current_layer])
      state.available_hours =
        api.GetAvailableForecastHours(
          LayerAt(state.current_layer).api_parameter);
  } else {
    for (unsigned i = 0; i < Region().layer_count; ++i)
      state.layer_available[i] = false;
  }
}

void
XCThermControlsModel::SyncActiveLayerFromSettings() noexcept
{
  auto &api = XCThermAPI::Instance();
  const int active_layer = FindActiveLayerIndex(Settings());

  if (active_layer == state.last_synced_active_layer) {
    MaybeFetchActiveLayer(nullptr);
    return;
  }

  state.last_synced_active_layer = active_layer;
  ResetAutoFetchAttempt();

  if (active_layer >= 0) {
    const unsigned active = unsigned(active_layer);
    const std::string &active_param = LayerAt(active).api_parameter;

    if (!api.GetCachedHours(active_param).empty()) {
      state.current_layer = active;
    } else if (state.current_layer == active) {
      const int fallback = FindFirstCachedLayerIndex(Settings());
      if (fallback >= 0)
        state.current_layer = unsigned(fallback);
    }

    MaybeFetchActiveLayer(nullptr);
    return;
  }

  const int fallback = FindFirstCachedLayerIndex(Settings());
  if (fallback >= 0)
    state.current_layer = unsigned(fallback);
}

void
XCThermControlsModel::RefreshCachedHours() noexcept
{
  state.cached_hours =
    XCThermAPI::Instance().GetCachedHours(
      LayerAt(state.current_layer).api_parameter);
}

void
XCThermControlsModel::RefreshAvailableHours() noexcept
{
  if (!state.index_loaded)
    return;

  state.available_hours =
    XCThermAPI::Instance().GetAvailableForecastHours(
      LayerAt(state.current_layer).api_parameter);
}

void
XCThermControlsModel::SelectBestTimeIndex() noexcept
{
  const auto utc = GetUtcTimeParts();
  state.current_time_index = PickAutoTargetUtcHour(utc.hour, utc.minute);
}

void
XCThermControlsModel::ApplyCurrentSelectionToMap() noexcept
{
  if (!HasCacheAtCurrentHour(state.current_layer)) {
    ClearMapOverlay();
    return;
  }

  ApplyLayerToMap(state.current_layer, GetCurrentForecastHour());
}

bool
XCThermControlsModel::StepLayer(int delta) noexcept
{
  auto_switch.OnManualLayerStep();

  const unsigned layer_count = Region().layer_count;
  if (layer_count == 0)
    return false;

  int pos = int(state.current_layer) + delta;
  if (pos < 0)
    pos = int(layer_count) - 1;
  else if (pos >= int(layer_count))
    pos = 0;
  state.current_layer = unsigned(pos);

  RefreshCachedHours();
  RefreshAvailableHours();
  SelectBestTimeIndex();
  ApplyCurrentSelectionToMap();
  auto_switch.SyncCurrentLayerIndex(state.current_layer);
  SaveCursorSession();
  return true;
}

bool
XCThermControlsModel::StepTime(int delta) noexcept
{
  auto_switch.OnManualTimeStep();

  int hour = int(GetCurrentForecastHour()) + delta;
  hour = (hour % 24 + 24) % 24;
  state.current_time_index = unsigned(hour);

  ApplyCurrentSelectionToMap();
  SaveCursorSession();
  return true;
}

void
XCThermControlsModel::ResumeLayerAuto(const MoreData &basic) noexcept
{
  if (!auto_switch.IsEnabled() || auto_switch.IsAltitudeAutoActive())
    return;

  auto_switch.ResumeAltitudeAuto(BestAltitude(basic));
  SaveCursorSession();
}

void
XCThermControlsModel::ResumeTimeAuto() noexcept
{
  if (!auto_switch.IsEnabled() || auto_switch.IsTimeAutoActive())
    return;

  auto_switch.ResumeTimeAuto();

  const auto utc = GetUtcTimeParts();
  state.current_time_index = PickAutoTargetUtcHour(utc.hour, utc.minute);
  ApplyCurrentSelectionToMap();
  SaveCursorSession();
}

void
XCThermControlsModel::OnIndexLoaded() noexcept
{
  ApplyIndexAvailability();
  if (!state.index_loaded) {
    LogFmt("xctherm: index not loaded — controls in offline mode");
    return;
  }

  LogFmt("xctherm: index loaded — refreshing controls");
  if (!CommonInterface::GetUIState().weather.xctherm.cursor_initialized)
    SyncActiveLayerFromSettings();
  RefreshCachedHours();
  if (!CommonInterface::GetUIState().weather.xctherm.cursor_initialized)
    SelectBestTimeIndex();
  ApplyCurrentSelectionToMap();
  MaybeFetchActiveLayer(nullptr);
  ConfigureAutoSwitch();
  SaveCursorSession();
}

void
XCThermControlsModel::RequestBackgroundIndex(
  std::function<void()> on_ready) const
{
  if (state.index_loaded) {
    if (on_ready)
      on_ready();
    return;
  }

  RequestBackgroundIndexFetch(std::move(on_ready));
}

void
XCThermControlsModel::MaybeFetchActiveLayer(
  std::function<void(std::shared_ptr<XCThermDownloadJob>)> on_finished) const
{
  if (!state.index_loaded)
    return;

  MaybeFetchActiveLayerSpan(std::move(on_finished));
}

void
XCThermControlsModel::OnDownloadFinished(
  const std::shared_ptr<XCThermDownloadJob> &job) noexcept
{
  RefreshCachedHours();

  if (job == nullptr || job->cancel.load())
    return;

  if (!job->error_eptr && job->succeeded_or_cached.load() > 0) {
    if (!CommonInterface::GetUIState().weather.xctherm.cursor_initialized)
      SelectBestTimeIndex();
    ApplyCurrentSelectionToMap();
    SaveCursorSession();
  }
}

void
XCThermControlsModel::FormatLayerLabel(StaticString<80> &text) const noexcept
{
  const unsigned layer = state.current_layer;
  const bool has_cache = HasCacheAtCurrentHour(layer);

  if (!has_cache)
    WeatherMapOverlay::AppendNoDataTag(text,
                                       gettext(LayerAt(layer).short_label));
  else if (auto_switch.IsAltitudeAutoActive())
    text.Format("%s %s", _("AUTO:"), gettext(LayerAt(layer).short_label));
  else
    text.Format("%s", gettext(LayerAt(layer).short_label));
}

void
XCThermControlsModel::FormatTimeLabel(StaticString<64> &text) noexcept
{
  RefreshCachedHours();
  RefreshAvailableHours();

  StaticString<64> base;
  XCTherm::FormatTimeLabel(base,
                           LayerAt(state.current_layer).api_parameter,
                           GetCurrentForecastHour(),
                           auto_switch.IsTimeAutoActive());

  if (HasCacheAtCurrentHour(state.current_layer))
    text = base;
  else
    WeatherMapOverlay::AppendNoDataTag(text, base.c_str());
}

bool
XCThermControlsModel::IsAltitudeAutoActive() const noexcept
{
  return auto_switch.IsAltitudeAutoActive();
}

bool
XCThermControlsModel::IsTimeAutoActive() const noexcept
{
  return auto_switch.IsTimeAutoActive();
}

bool
XCThermControlsModel::LayerHasCache(unsigned layer_index) const noexcept
{
  return !XCThermAPI::Instance()
    .GetCachedHours(LayerAt(layer_index).api_parameter).empty();
}

bool
XCThermControlsModel::LayerUsableForAutoSwitch(
  unsigned layer_index) const noexcept
{
  return state.layer_available[layer_index] || LayerHasCache(layer_index);
}

std::vector<XCThermAutoSwitch::LayerInfo>
XCThermControlsModel::BuildAutoSwitchLayers() const noexcept
{
  std::vector<XCThermAutoSwitch::LayerInfo> layers;
  for (unsigned i = 0; i < Region().layer_count; ++i) {
    if (LayerUsableForAutoSwitch(i))
      layers.push_back({i, LayerAt(i).altitude_m, LayerAt(i).is_agl});
  }
  return layers;
}

bool
XCThermControlsModel::HasCacheAtCurrentHour(unsigned layer_index) const noexcept
{
  return XCThermAPI::Instance().IsLayerCached(
    LayerAt(layer_index).api_parameter, GetCurrentForecastHour());
}

} // namespace XCTherm
