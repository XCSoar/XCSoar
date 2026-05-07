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
class SkySightClient;
class SkySightRequest;

class SkySightAPI final {
  SkySightClient &owner;
  std::unique_ptr<SkySightRequest> request;
  std::vector<SkySight::Layer> layers;
  const AllocatedPath cache_path;
  std::string region = GetDefaultSkySightRegion().id;

public:
  SkySightAPI(SkySightClient &_owner, CurlGlobal &curl, Path _cache_path);
  ~SkySightAPI();

  void Configure(std::string_view email, std::string_view password,
                 std::string_view new_region);

  bool HasCredentials() const noexcept;

  std::size_t NumLayers() const noexcept {
    return layers.size();
  }

  const SkySight::Layer *GetLayer(std::size_t index) const noexcept;
  SkySight::Layer *GetLayer(std::string_view id) noexcept;

  AllocatedPath GetTilePath(const SkySight::Layer &layer, time_t timestamp,
                            const GeoBitmap::TileData &tile) const;
  void EnsureTile(const SkySight::Layer &layer, time_t timestamp,
                  const GeoBitmap::TileData &tile);
  void OnLiveTileProbeSucceeded(std::string_view layer_id,
                                time_t timestamp) noexcept;
  void PollLastUpdates() noexcept;
  void ResetLastUpdates() noexcept;

  void OnAuthenticated() noexcept;
  void OnLastUpdates(std::string_view requested_layer_id,
                     boost::json::value value) noexcept;
  void OnDownloadComplete() noexcept;

private:
  static void InitialiseLayers(std::vector<SkySight::Layer> &layers);
  static std::string FormatUrlTimestamp(time_t timestamp);
  static std::string FormatFileTimestamp(time_t timestamp);
  static std::string MakeTileUrl(const SkySight::Layer &layer,
                                 time_t timestamp,
                                 const GeoBitmap::TileData &tile);
};
