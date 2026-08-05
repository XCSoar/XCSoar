// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"
#include "SkySightCache.hpp"
#include "MapWindow/OverlayLimits.hpp"
#include "Geo/GeoBounds.hpp"
#include "time/RoughTime.hpp"
#include "ui/canvas/custom/GeoBitmap.hpp"
#include "system/Path.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct SkySightRegionEntry;

class CurlGlobal;
struct PageLayout;
class Path;
class SkySightAPI;

class SkySightClient final {
  static constexpr unsigned LIVE_TILE_RANGE_OFFSET = 2;
  static constexpr unsigned LIVE_TILE_OVERLAY_COUNT =
    (2 * LIVE_TILE_RANGE_OFFSET + 1) * (2 * LIVE_TILE_RANGE_OFFSET + 1);
  static_assert(LIVE_TILE_OVERLAY_COUNT <= MapWindowOverlay::MAX_MAP_OVERLAYS);

  std::unique_ptr<SkySightAPI> api;
  SkySight::Layer *active_layer = nullptr;
  SkySight::Layer *displayed_layer = nullptr;
  bool forecast_image_dirty = true;
  bool forecast_cleanup_pending = true;
  bool forecast_progress_visible = false;
  bool throttle_notification_active = false;
  bool manual_update_requested = false;
  bool planned_live_timestamp_known = false;
  time_t planned_live_timestamp = 0;
  GeoBounds planned_live_bounds = GeoBounds::Invalid();
  std::vector<GeoBitmap::TileData> planned_live_tiles;
  std::array<std::string, LIVE_TILE_OVERLAY_COUNT> tile_filenames;
  std::array<GeoBitmap::TileData, LIVE_TILE_OVERLAY_COUNT> tile_coordinates;
  std::array<time_t, LIVE_TILE_OVERLAY_COUNT> tile_timestamps{};
  std::chrono::steady_clock::time_point next_cleanup_check;
  UI::PeriodicTimer request_timer;

public:
  explicit SkySightClient(CurlGlobal &curl);
  ~SkySightClient();

  /** Stop polling and cancel network/decode work before teardown. */
  void BeginShutdown() noexcept;

  void Init();

  [[nodiscard]] static AllocatedPath GetCachePath() noexcept;

  std::size_t NumLayers() const noexcept;
  const SkySight::Layer *GetLayer(std::size_t index) const noexcept;
  const std::vector<SkySightRegionEntry> &GetRegions() const noexcept;
  std::string_view GetRegion() const noexcept;
  /** IANA tz for the active SkySight region, or empty if unknown. */
  [[nodiscard]] std::string_view GetRegionTimeZone() const noexcept;
  /**
   * Region civil UTC offset for a UTC forecast/live instant.
   * Used only at UI/URL edges; internal state stays UTC.
   */
  [[nodiscard]] RoughTimeDelta
  GetForecastDisplayOffset(time_t utc_time) const noexcept;
  std::size_t NumSelectedLayers() const noexcept;
  const SkySight::Layer *GetSelectedLayer(std::size_t index) const noexcept;
  const SkySight::Layer *GetSelectedLayer(std::string_view id) const noexcept;
  bool IsSelectedLayer(std::string_view id) const noexcept;
  bool SelectedLayersFull() const noexcept;
  bool AddSelectedLayer(std::string_view id);
  bool RemoveSelectedLayer(std::string_view id);
  bool SelectForecastTime(std::string_view id, time_t forecast_time,
                          bool download=true);
  bool SelectAutomaticForecastTime(std::string_view id,
                                   bool download=true);
  bool PreloadForecast(std::string_view id) noexcept;
  bool PreloadAllForecasts() noexcept;
  unsigned GetPreloadFileCount() const;
  unsigned GetSelectedForecastLayerCount() const noexcept;
  bool HasForecastLayers() const noexcept;
  bool IsForecastDecodeAvailable() const noexcept;
  void RefreshCatalog() noexcept;
  [[nodiscard]] SkySightCache::Usage GetCacheUsage() const noexcept;
  [[nodiscard]] SkySightCache::Usage ClearDownloadedData() noexcept;

  bool HasCredentials() const noexcept;
  bool IsAutoUpdateEnabled() const noexcept;
  void OnAutoUpdateChanged() noexcept;

  bool IsThrottled() const noexcept;

  bool IsLiveViewUpdating(std::string_view layer_id) const noexcept;
  bool HasDownloadActivity() const noexcept;

  time_t GetThrottleRemainingSeconds() const noexcept;
  time_t GetDatafilesRetryRemainingSeconds() const noexcept;

  std::string_view GetActiveLayerId() const noexcept;
  std::string_view GetDisplayedLayerId() const noexcept;

  bool SetLayerActive(std::string_view id, bool request_update=false);
  void ApplyPageOverlay(const PageLayout &page) noexcept;
  void DeactivateLayer();
  void Render();

  void ReloadSelectedLayersFromProfile();
  void OnLayerCatalogChanged(std::string_view active_id,
                             std::string_view displayed_id) noexcept;
  void OnDataUpdated() noexcept;
  void OnForecastThrottled() noexcept;
  void OnForecastResumed() noexcept;
  void OnForecastProgress(const SkySight::ForecastProgress &progress) noexcept;
  void OnForecastProgressCancelled() noexcept;

private:
  bool AddSelectedLayer(std::string_view id, bool save_profile,
                        bool request_datafiles);
  void MaybeCleanupFiles() noexcept;
  void SaveSelectedLayers() const;
  [[nodiscard]] bool CleanupFiles() noexcept;
  void ResetTiles() noexcept;
  bool UpdateActiveLayer(unsigned index, Path path,
                         const GeoBitmap::TileData &tile);
  bool DisplayForecastLayer();
  bool DisplayTileLayer();
};
