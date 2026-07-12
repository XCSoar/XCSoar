// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"
#include "ui/canvas/custom/GeoBitmap.hpp"
#include "system/Path.hpp"
#include "util/StaticString.hxx"
#include "ui/event/PeriodicTimer.hpp"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct SkysightRegionEntry;

class CurlGlobal;
class Path;
class SkysightAPI;

class Skysight final {
  std::unique_ptr<SkysightAPI> api;
  SkySight::Layer *active_layer = nullptr;
  SkySight::Layer *displayed_layer = nullptr;
  unsigned displayed_zoom = 0;
  bool forecast_image_dirty = true;
  bool forecast_cleanup_pending = true;
  bool forecast_progress_visible = false;
  bool throttle_notification_active = false;
  std::array<std::string, 9> tile_filenames;
  UI::PeriodicTimer request_timer;

public:
  explicit Skysight(CurlGlobal &curl);
  ~Skysight();

  void Init();

  [[nodiscard]] static AllocatedPath GetLocalPath() noexcept;

  std::size_t NumLayers() const noexcept;
  const SkySight::Layer *GetLayer(std::size_t index) const noexcept;
  const std::vector<SkysightRegionEntry> &GetRegions() const noexcept;
  std::string_view GetRegion() const noexcept;
  std::size_t NumSelectedLayers() const noexcept;
  const SkySight::Layer *GetSelectedLayer(std::size_t index) const noexcept;
  const SkySight::Layer *GetSelectedLayer(std::string_view id) const noexcept;
  bool IsSelectedLayer(std::string_view id) const noexcept;
  bool SelectedLayersFull() const noexcept;
  bool AddSelectedLayer(std::string_view id);
  bool RemoveSelectedLayer(std::string_view id);
  bool SelectForecastTime(std::string_view id, time_t forecast_time);
  bool SelectAutomaticForecastTime(std::string_view id);
  bool SelectPageLayer(std::string_view id);
  bool PreloadForecast(std::string_view id) noexcept;
  bool PreloadAllForecasts() noexcept;
  unsigned GetPreloadFileCount() const noexcept;
  unsigned GetSelectedForecastLayerCount() const noexcept;
  bool HasForecastLayers() const noexcept;
  bool IsForecastDecodeAvailable() const noexcept;
  void RefreshCatalog() noexcept;
  void PollPendingDatafiles() noexcept;

  bool HasCredentials() const noexcept;

  bool IsThrottled() const noexcept;

  time_t GetThrottleRemainingSeconds() const noexcept;

  std::string_view GetActiveLayerId() const noexcept;
  std::string_view GetDisplayedLayerId() const noexcept;
  StaticString<128> GetOverlayLabel() const noexcept;

  bool SetLayerActive(std::string_view id);
  void ApplyPageOverlay(std::string_view overlay_id) noexcept;
  void DeactivateLayer();
  void Render();

  void ReloadSelectedLayersFromProfile();
  void OnLayerCatalogChanged(std::string_view active_id,
                             std::string_view displayed_id) noexcept;
  void OnDataUpdated() noexcept;
  void OnForecastThrottled() noexcept;
  void OnForecastResumed() noexcept;
  void OnForecastProgress(const SkySight::ForecastProgress &progress) noexcept;

private:
  bool AddSelectedLayer(std::string_view id, bool save_profile);
  void MaybeCleanupFiles() noexcept;
  void SaveSelectedLayers() const;
  void CleanupFiles() noexcept;
  void ResetTiles() noexcept;
  bool UpdateActiveLayer(unsigned index, Path path,
                         const GeoBitmap::TileData &tile);
  bool DisplayForecastLayer();
  bool DisplayTileLayer();
};
