// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"
#include "Regions.hpp"
#include "SkySightFileDecoder.hpp"
#include "system/Path.hpp"
#include "ui/canvas/custom/GeoBitmap.hpp"

#include <boost/json.hpp>

#include <ctime>
#include <deque>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class CurlGlobal;
class Skysight;
class SkySightRequest;

class SkysightAPI final {
  Skysight &owner;
  std::unique_ptr<SkySightRequest> request;
  std::unique_ptr<SkySightFileDecodeJob> decode_job;
  struct PendingDecodeJob {
    SkySightPreparedData prepared;
    std::string variable_name;
    std::map<float, SkySight::LegendColor> legend;
    std::string layer_id;
    time_t forecast_time = 0;
  };
  std::deque<PendingDecodeJob> pending_decode_jobs;
  std::vector<SkySight::Layer> layers;
  std::vector<SkySight::Layer> selected_layers;
  std::vector<SkysightRegionEntry> regions = GetDefaultSkysightRegions();
  const AllocatedPath cache_path;
  std::string region = GetDefaultSkysightRegion().id;

  bool PreloadDatafiles(std::string_view layer_id,
                        bool begin_progress) noexcept;

public:
  SkysightAPI(Skysight &_owner, CurlGlobal &curl, Path _cache_path);
  ~SkysightAPI();

  void Configure(std::string_view email, std::string_view password,
                 std::string_view new_region);

  bool HasCredentials() const noexcept;
  bool IsThrottled() const noexcept;

  time_t GetThrottleRemainingSeconds() const noexcept;
  time_t GetDatafilesRetryRemainingSeconds() const noexcept;
  void Poll() noexcept;

  const std::vector<SkysightRegionEntry> &GetRegions() const noexcept {
    return regions;
  }

  std::string_view GetRegion() const noexcept {
    return region;
  }

  std::size_t NumLayers() const noexcept;

  const SkySight::Layer *GetLayer(std::size_t index) const noexcept;
  SkySight::Layer *GetLayer(std::string_view id) noexcept;
  std::size_t NumSelectedLayers() const noexcept {
    return selected_layers.size();
  }

  const SkySight::Layer *GetSelectedLayer(std::size_t index) const noexcept;
  SkySight::Layer *GetSelectedLayer(std::string_view id) noexcept;
  bool IsSelectedLayer(std::string_view id) const noexcept;
  bool SelectedLayersFull() const noexcept;
  bool AddSelectedLayer(const SkySight::Layer &layer);
  bool RemoveSelectedLayer(std::string_view id) noexcept;
  void ClearSelectedLayers() noexcept;

  AllocatedPath GetTilePath(const SkySight::Layer &layer, time_t timestamp,
                            const GeoBitmap::TileData &tile) const;
  AllocatedPath GetDatafilePath(const SkySight::Layer &layer,
                                time_t forecast_time,
                                std::string_view suffix) const;
  void EnsureTile(const SkySight::Layer &layer, time_t timestamp,
                  const GeoBitmap::TileData &tile);
  void CancelTileDownloads() noexcept;
  void EnsureDatafile(const SkySight::Layer &layer, time_t forecast_time,
                      std::string_view link);
  bool QueuePreloadDatafile(SkySight::Layer &layer, time_t forecast_time,
                            std::string_view link) noexcept;
  bool QueueForecastDatafile(std::string_view layer_id, time_t forecast_time,
                             std::string_view link) noexcept;
  bool PreloadDefaultDatafile(std::string_view layer_id) noexcept;
  bool PreloadDatafiles(std::string_view layer_id) noexcept;
  bool PreloadAllDatafiles() noexcept;
  void BeginPreloadProgress() noexcept;
  void AddPreloadTarget(std::string_view layer_id, time_t forecast_time) noexcept;
  void FinishPreloadTarget(std::string_view layer_id, time_t forecast_time,
                           bool failed=false) noexcept;
  void FinishPreloadMetadata(std::string_view layer_id, bool failed=false) noexcept;
  void UpdatePreloadProgress() noexcept;
  void PollRegions() noexcept;
  void PollLayers() noexcept;
  void PollLastUpdates() noexcept;
  void PollSelectedDatafiles() noexcept;
  void ResetRegions() noexcept;
  void ResetLastUpdates() noexcept;

  void OnAuthenticated() noexcept;
  void OnRegions(boost::json::value value) noexcept;
  void OnLayers(boost::json::value value) noexcept;
  void OnLastUpdates(boost::json::value value) noexcept;
  void OnDatafiles(std::string_view layer_id, boost::json::value value) noexcept;
  void OnDatafilesRetry(std::string_view layer_id) noexcept;
  void OnDatafilesError(std::string_view layer_id) noexcept;
  void OnDatafileDownloaded(std::string_view layer_id, time_t forecast_time,
                            SkySightPreparedData prepared) noexcept;
  void OnDatafileError(std::string_view layer_id, time_t forecast_time,
                       bool preparation_failed=false) noexcept;
  void OnDownloadComplete() noexcept;
  void OnThrottle() noexcept;
  void OnThrottleEnded() noexcept;

private:
  static constexpr std::size_t MAX_SELECTED_LAYERS = 8;

  struct PreloadTarget {
    std::string layer_id;
    time_t forecast_time;
    bool finished = false;
  };

  std::vector<PreloadTarget> preload_targets;
  std::vector<std::string> preload_metadata_layers;
  unsigned preload_failures = 0;
  bool preload_progress_active = false;
  bool preload_progress_initializing = false;

  bool regions_loaded = false;
  time_t last_regions_request = 0;
  time_t last_regions_refresh = 0;
  bool layers_loaded = false;
  time_t last_layers_request = 0;
  time_t last_layers_refresh = 0;
  time_t last_updates_request = 0;

  void SyncSelectedLayer(std::string_view id) noexcept;
  static void UpdateBusyState(SkySight::Layer &layer) noexcept;
  [[gnu::pure]] AllocatedPath GetRegionsCachePath() const noexcept;
  [[gnu::pure]] AllocatedPath GetLayersCachePath() const noexcept;
  void LoadCachedRegions() noexcept;
  void LoadCachedLayers() noexcept;
  bool ParseRegions(const boost::json::value &value,
                    const char *error_context) noexcept;
  bool ParseLayers(const boost::json::value &value,
                   const char *error_context) noexcept;
  void StoreMetadataCache(Path path,
                          const boost::json::value &value) const noexcept;
  bool QueueForecastDatafile(SkySight::Layer &layer, time_t forecast_time,
                             std::string_view link) noexcept;
  bool QueueDecodeJob(SkySightPreparedData prepared,
                      const SkySight::Layer &layer,
                      time_t forecast_time) noexcept;
  void StartNextDecodeJob() noexcept;
  static void InitialiseLayers(std::vector<SkySight::Layer> &layers);
  static std::string FormatUrlTimestamp(time_t timestamp);
  static std::string FormatFileTimestamp(time_t timestamp);
  static std::string MakeTileUrl(const SkySight::Layer &layer,
                                 time_t timestamp,
                                 const GeoBitmap::TileData &tile);
};
