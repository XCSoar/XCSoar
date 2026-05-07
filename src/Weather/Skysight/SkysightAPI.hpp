// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Layers.hpp"
#include "Regions.hpp"
#include "system/Path.hpp"
#include "ui/canvas/custom/GeoBitmap.hpp"

#include <boost/json.hpp>

#include <ctime>
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
  std::vector<SkySight::Layer> layers;
  std::vector<SkySight::Layer> selected_layers;
  std::vector<SkysightRegionEntry> regions = GetDefaultSkysightRegions();
  const AllocatedPath cache_path;
  std::string region = GetDefaultSkysightRegion().id;

public:
  SkysightAPI(Skysight &_owner, CurlGlobal &curl, Path _cache_path);
  ~SkysightAPI();

  void Configure(std::string_view email, std::string_view password,
                 std::string_view new_region);

  bool HasCredentials() const noexcept;

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
  void EnsureTile(const SkySight::Layer &layer, time_t timestamp,
                  const GeoBitmap::TileData &tile);
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
  void OnDatafilesError(std::string_view layer_id) noexcept;
  void OnDownloadComplete() noexcept;

private:
  static constexpr std::size_t MAX_SELECTED_LAYERS = 8;

  bool regions_loaded = false;
  time_t last_regions_request = 0;
  bool layers_loaded = false;
  time_t last_layers_request = 0;
  time_t last_updates_request = 0;

  void SyncSelectedLayer(std::string_view id) noexcept;
  static void InitialiseLayers(std::vector<SkySight::Layer> &layers);
  static std::string FormatUrlTimestamp(time_t timestamp);
  static std::string FormatFileTimestamp(time_t timestamp);
  static std::string MakeTileUrl(const SkySight::Layer &layer,
                                 time_t timestamp,
                                 const GeoBitmap::TileData &tile);
};