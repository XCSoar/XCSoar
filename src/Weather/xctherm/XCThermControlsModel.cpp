// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "XCThermControlsModel.hpp"
#include "XCThermAPI.hpp"
#include "XCThermDownloadJob.hpp"
#include "XCThermForecastTime.hpp"
#include "XCThermMapOverlay.hpp"
#include "Interface.hpp"
#include "LogFile.hpp"
#include "Weather/Settings.hpp"

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

void
XCThermControlsModel::ApplyLayerToMap(unsigned layer_index,
                                      unsigned utc_hour) noexcept
{
  ApplyCachedLayerOverlay(layer_index, utc_hour);
}

unsigned
XCThermControlsModel::GetCurrentForecastHour() const noexcept
{
  return ForecastHourAt(state.cached_hours, state.current_time_index,
                        state.available_hours);
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

  const int def = FindLayerIndex(ToRegion(settings.model), 5000, false);
  if (def >= 0)
    state.current_layer = unsigned(def);

  SyncActiveLayerFromSettings();
  RefreshCachedHours();
  SelectBestTimeIndex();
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
    if (!HasMapOverlay())
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
XCThermControlsModel::SelectBestTimeIndex() noexcept
{
  if (state.cached_hours.empty())
    return;

  const auto utc = GetUtcTimeParts();
  const int idx = PickAutoTimeIndex(state.cached_hours, utc.hour, utc.minute);
  if (idx >= 0)
    state.current_time_index = unsigned(idx);
}

void
XCThermControlsModel::ApplyCurrentSelectionToMap() noexcept
{
  if (state.cached_hours.empty())
    return;
  ApplyLayerToMap(state.current_layer, GetCurrentForecastHour());
}

bool
XCThermControlsModel::StepLayer(int delta) noexcept
{
  const auto cached = CollectCachedLayerIndices();
  if (cached.empty())
    return false;

  auto it = std::find(cached.begin(), cached.end(), state.current_layer);
  int pos = (it != cached.end()) ? int(it - cached.begin()) : 0;
  pos += delta;
  if (pos < 0)
    pos = int(cached.size()) - 1;
  if (pos >= int(cached.size()))
    pos = 0;
  state.current_layer = cached[unsigned(pos)];

  RefreshCachedHours();
  SelectBestTimeIndex();
  ApplyCurrentSelectionToMap();
  return true;
}

bool
XCThermControlsModel::StepTime(int delta) noexcept
{
  RefreshCachedHours();
  if (state.cached_hours.empty())
    return false;

  int next = int(state.current_time_index) + delta;
  if (next < 0)
    next = int(state.cached_hours.size()) - 1;
  if (next >= int(state.cached_hours.size()))
    next = 0;
  state.current_time_index = unsigned(next);

  ApplyLayerToMap(state.current_layer,
                  state.cached_hours[state.current_time_index]);
  return true;
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
  SyncActiveLayerFromSettings();
  RefreshCachedHours();
  SelectBestTimeIndex();
  ApplyCurrentSelectionToMap();
  MaybeFetchActiveLayer(nullptr);
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
    SelectBestTimeIndex();
    ApplyCurrentSelectionToMap();
  }
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

std::vector<unsigned>
XCThermControlsModel::CollectCachedLayerIndices() const noexcept
{
  std::vector<unsigned> cached;
  for (unsigned i = 0; i < Region().layer_count; ++i) {
    if (LayerHasCache(i))
      cached.push_back(i);
  }
  return cached;
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
