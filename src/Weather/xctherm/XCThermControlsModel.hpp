// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/xctherm/XCThermAutoSwitch.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"
#include "util/StaticString.hxx"

#include <functional>
#include <memory>
#include <vector>

struct MoreData;
struct XCThermDownloadJob;
struct XCThermSettings;

namespace XCTherm {

/**
 * Cursor-bar state: layer/time selection, index availability, cache hours,
 * and auto-switch orchestration.  UI-agnostic; used by #XCThermControlsWidget.
 */
struct XCThermControlsState {
  unsigned current_layer = 0;
  /** Selected forecast UTC hour (0–23) for cursor bar and map. */
  unsigned current_time_index = 0;
  bool layer_available[MaxLayerCount()] = {};
  std::vector<unsigned> cached_hours;
  std::vector<unsigned> available_hours;
  bool index_loaded = false;
  int last_synced_active_layer = -1;
};

class XCThermControlsModel {
  XCThermControlsState state;
  XCThermAutoSwitch auto_switch;
  bool auto_switch_configured = false;
  std::function<void()> on_state_changed;

  void NotifyStateChanged() noexcept;

  [[gnu::pure]]
  const RegionDef &Region() const noexcept;

  [[gnu::pure]]
  const Layer &LayerAt(unsigned index) const noexcept;

  [[gnu::pure]]
  const XCThermSettings &Settings() const noexcept;

  [[gnu::pure]]
  static double BestAltitude(const MoreData &basic) noexcept;

  void ConfigureAutoSwitch() noexcept;
  void ApplyAutoSwitchLayer(unsigned layer_index) noexcept;

public:
  const XCThermControlsState &GetState() const noexcept {
    return state;
  }

  unsigned GetCurrentLayer() const noexcept {
    return state.current_layer;
  }

  unsigned GetCurrentTimeIndex() const noexcept {
    return state.current_time_index;
  }

  void SetCurrentLayer(unsigned layer) noexcept {
    state.current_layer = layer;
  }

  void SetCurrentTimeIndex(unsigned index) noexcept {
    state.current_time_index = index;
  }

  bool IsLayerAvailable(unsigned index) const noexcept {
    return index < Region().layer_count && state.layer_available[index];
  }

  bool IsIndexLoaded() const noexcept { return state.index_loaded; }

  const std::vector<unsigned> &GetCachedHours() const noexcept {
    return state.cached_hours;
  }

  const std::vector<unsigned> &GetAvailableHours() const noexcept {
    return state.available_hours;
  }

  unsigned GetCurrentForecastHour() const noexcept;

  /** Widget lifetime: bootstrap session and background fetches. */
  void Prepare(std::function<void()> on_state_changed = nullptr) noexcept;

  void OnShow() noexcept;
  void OnHide() noexcept;

  /** Periodic blackboard tick (auto-switch, settings sync). */
  void OnGPSUpdate(const MoreData &basic) noexcept;

  void BootstrapSession() noexcept;

  void ApplyIndexAvailability() noexcept;

  void SyncActiveLayerFromSettings() noexcept;

  void RefreshCachedHours() noexcept;

  void RefreshAvailableHours() noexcept;

  void LoadCursorSession() noexcept;
  void SaveCursorSession() noexcept;

  void SelectBestTimeIndex() noexcept;

  void ApplyCurrentSelectionToMap() noexcept;

  void ApplyLayerToMap(unsigned layer_index, unsigned utc_hour) noexcept;

  /**
   * Step through all altitude bands for the region.
   * @return false when the region defines no bands.
   */
  bool StepLayer(int delta) noexcept;

  /** Step through index forecast hours, or 24 UTC hours when unknown. */
  bool StepTime(int delta) noexcept;

  void ResumeLayerAuto(const MoreData &basic) noexcept;
  void ResumeTimeAuto() noexcept;

  void OnIndexLoaded() noexcept;

  void RequestBackgroundIndex(std::function<void()> on_ready) const;

  void MaybeFetchActiveLayer(
    std::function<void(std::shared_ptr<XCThermDownloadJob>)> on_finished) const;

  /**
   * Queue a span download for the cursor-bar layer.
   *
   * @return true if a download was started
   */
  bool RequestLayerDownload(
    std::function<void(std::shared_ptr<XCThermDownloadJob>)> on_finished
    = nullptr) noexcept;

  void OnDownloadFinished(const std::shared_ptr<XCThermDownloadJob> &job) noexcept;

  void FormatLayerLabel(StaticString<80> &text) const noexcept;
  void FormatTimeLabel(StaticString<64> &text) noexcept;

  [[gnu::pure]]
  bool IsAltitudeAutoActive() const noexcept;

  [[gnu::pure]]
  bool IsTimeAutoActive() const noexcept;

  [[gnu::pure]]
  bool LayerHasCache(unsigned layer_index) const noexcept;

  [[gnu::pure]]
  bool LayerUsableForAutoSwitch(unsigned layer_index) const noexcept;

  [[gnu::pure]]
  std::vector<XCThermAutoSwitch::LayerInfo> BuildAutoSwitchLayers() const noexcept;

  [[gnu::pure]]
  bool HasCacheAtCurrentHour(unsigned layer_index) const noexcept;
};

} // namespace XCTherm
