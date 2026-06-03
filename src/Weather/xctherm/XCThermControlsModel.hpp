// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/xctherm/XCThermAutoSwitch.hpp"
#include "Weather/xctherm/XCThermCatalog.hpp"

#include <functional>
#include <memory>
#include <vector>

struct XCThermDownloadJob;
struct XCThermSettings;

namespace XCTherm {

/**
 * Cursor-bar state: layer/time selection, index availability, cache hours.
 * UI-agnostic; used by #XCThermControlsWidget.
 */
struct XCThermControlsState {
  unsigned current_layer = 0;
  unsigned current_time_index = 0;
  bool layer_available[MaxLayerCount()] = {};
  std::vector<unsigned> cached_hours;
  std::vector<unsigned> available_hours;
  bool index_loaded = false;
  int last_synced_active_layer = -1;
};

class XCThermControlsModel {
  XCThermControlsState state;

  [[gnu::pure]]
  const RegionDef &Region() const noexcept;

  [[gnu::pure]]
  const Layer &LayerAt(unsigned index) const noexcept;

  [[gnu::pure]]
  const XCThermSettings &Settings() const noexcept;

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

  void BootstrapSession() noexcept;

  void ApplyIndexAvailability() noexcept;

  void SyncActiveLayerFromSettings() noexcept;

  void RefreshCachedHours() noexcept;

  void SelectBestTimeIndex() noexcept;

  void ApplyCurrentSelectionToMap() noexcept;

  void ApplyLayerToMap(unsigned layer_index, unsigned utc_hour) noexcept;

  /**
   * @return false when no cached layers exist to step through.
   */
  bool StepLayer(int delta) noexcept;

  /**
   * @return false when the current layer has no cached hours.
   */
  bool StepTime(int delta) noexcept;

  void OnIndexLoaded() noexcept;

  void RequestBackgroundIndex(std::function<void()> on_ready) const;

  void MaybeFetchActiveLayer(
    std::function<void(std::shared_ptr<XCThermDownloadJob>)> on_finished) const;

  void OnDownloadFinished(const std::shared_ptr<XCThermDownloadJob> &job) noexcept;

  [[gnu::pure]]
  bool LayerHasCache(unsigned layer_index) const noexcept;

  [[gnu::pure]]
  bool LayerUsableForAutoSwitch(unsigned layer_index) const noexcept;

  [[gnu::pure]]
  std::vector<unsigned> CollectCachedLayerIndices() const noexcept;

  [[gnu::pure]]
  std::vector<XCThermAutoSwitch::LayerInfo> BuildAutoSwitchLayers() const noexcept;

  [[gnu::pure]]
  bool HasCacheAtCurrentHour(unsigned layer_index) const noexcept;
};

} // namespace XCTherm
