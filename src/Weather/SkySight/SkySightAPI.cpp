// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightAPI.hpp"
#include "SkySightRequest.hpp"
#include "SkySightClient.hpp"
#include "time/Convert.hxx"
#include "util/StaticString.hxx"
#include "LogFile.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>

namespace {

static constexpr time_t REGIONS_RETRY_SECONDS = 30;
static constexpr time_t LAYERS_RETRY_SECONDS = 30;
static constexpr time_t INITIAL_LAST_UPDATE_POLL_SECONDS = 30;
static constexpr time_t LAST_UPDATE_POLL_SECONDS = 5 * 60;

static time_t
ParseUpdateTime(const boost::json::value &value)
{
  if (value.is_number())
    return value.to_number<time_t>();

  if (value.is_string())
    return std::strtoll(value.as_string().c_str(), nullptr, 10);

  return 0;
}

} // namespace

SkySightAPI::SkySightAPI(SkySightClient &_owner, CurlGlobal &curl, Path _cache_path)
  :owner(_owner),
   request(std::make_unique<SkySightRequest>(*this, curl)),
   cache_path(_cache_path)
{
  InitialiseLayers(layers);
}

SkySightAPI::~SkySightAPI() = default;

void
SkySightAPI::InitialiseLayers(std::vector<SkySight::Layer> &new_layers)
{
  new_layers.clear();

  new_layers.push_back({
    "satellite",
    "Satellite",
    "Live SkySight satellite tiles",
    true,
    true,
    1,
    8,
    1.0f,
  });

  new_layers.push_back({
    "rain",
    "Rain",
    "Live SkySight precipitation tiles",
    true,
    true,
    1,
    8,
    0.7f,
  });
}

void
SkySightAPI::Configure(std::string_view email, std::string_view password,
                       std::string_view new_region)
{
  region = FindSkySightRegionById(new_region.empty()
                                  ? std::string_view{GetDefaultSkySightRegion().id}
                                  : new_region).id;
  ResetRegions();
  layers_loaded = false;
  last_layers_request = 0;
  ResetLastUpdates();
  for (auto &layer : layers)
    layer.last_update = 0;

  request->Configure(email, password);
}

bool
SkySightAPI::HasCredentials() const noexcept
{
  return request->HasCredentials();
}

const SkySight::Layer *
SkySightAPI::GetLayer(std::size_t index) const noexcept
{
  return index < layers.size()
    ? &layers[index]
    : nullptr;
}

SkySight::Layer *
SkySightAPI::GetLayer(std::string_view id) noexcept
{
  for (auto &i : layers)
    if (i == id)
      return &i;

  return nullptr;
}

std::string
SkySightAPI::FormatUrlTimestamp(time_t timestamp)
{
  const auto tm = GmTime(std::chrono::system_clock::from_time_t(timestamp));

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y/%m/%d/%H%M", &tm);
  return buffer;
}

std::string
SkySightAPI::FormatFileTimestamp(time_t timestamp)
{
  const auto tm = GmTime(std::chrono::system_clock::from_time_t(timestamp));

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H%M", &tm);
  return buffer;
}

std::string
SkySightAPI::MakeTileUrl(const SkySight::Layer &layer,
                         time_t timestamp,
                         const GeoBitmap::TileData &tile)
{
  StaticString<256> url;
  url.Format("https://skysight.io/api/%s/%u/%u/%u/%s",
             layer.id.c_str(), tile.zoom, tile.x, tile.y,
             FormatUrlTimestamp(timestamp).c_str());
  return url.c_str();
}

AllocatedPath
SkySightAPI::GetTilePath(const SkySight::Layer &layer, time_t timestamp,
                         const GeoBitmap::TileData &tile) const
{
  StaticString<128> filename;
  filename.Format("%s-%u-%u-%u-%s.jpg",
                  layer.id.c_str(), tile.zoom, tile.x, tile.y,
                  FormatFileTimestamp(timestamp).c_str());
  return AllocatedPath::Build(cache_path, filename);
}

void
SkySightAPI::EnsureTile(const SkySight::Layer &layer, time_t timestamp,
                        const GeoBitmap::TileData &tile)
{
  request->DownloadFile(MakeTileUrl(layer, timestamp, tile),
                        GetTilePath(layer, timestamp, tile),
                        layer.requires_auth);
}

void
SkySightAPI::PollRegions() noexcept
{
  if (!HasCredentials() || regions_loaded)
    return;

  const auto now = std::time(nullptr);
  if (last_regions_request != 0 && now < last_regions_request + REGIONS_RETRY_SECONDS)
    return;

  last_regions_request = now;
  request->RequestRegions();
}

void
SkySightAPI::PollLayers() noexcept
{
  if (!HasCredentials() || region.empty() || layers_loaded)
    return;

  const auto now = std::time(nullptr);
  if (last_layers_request != 0 && now < last_layers_request + LAYERS_RETRY_SECONDS)
    return;

  last_layers_request = now;
  request->RequestLayers(region);
}

void
SkySightAPI::PollLastUpdates() noexcept
{
  if (!HasCredentials() || region.empty())
    return;

  const auto now = std::time(nullptr);
  const auto active_layer_id = owner.GetActiveLayerId();
  auto *active_layer = active_layer_id.empty()
    ? nullptr
    : GetLayer(active_layer_id);
  if (active_layer == nullptr)
    return;

  if (!active_layer->IsLiveMetadataPollDue(now,
                                           INITIAL_LAST_UPDATE_POLL_SECONDS,
                                           LAST_UPDATE_POLL_SECONDS))
    return;

  active_layer->last_update_request = now;
  request->RequestLastUpdates(region, active_layer->id);
}

void
SkySightAPI::ResetRegions() noexcept
{
  regions = GetDefaultSkySightRegions();
  regions_loaded = false;
  last_regions_request = 0;
}

void
SkySightAPI::ResetLastUpdates() noexcept
{
  for (auto &layer : layers)
    layer.last_update_request = 0;
}

void
SkySightAPI::OnAuthenticated() noexcept
{
  PollRegions();
  PollLayers();
  ResetLastUpdates();
  owner.OnDataUpdated();
}

void
SkySightAPI::OnRegions(boost::json::value value) noexcept
{
  try {
    std::vector<SkySightRegionEntry> new_regions;

    for (const auto &entry_value : value.as_array()) {
      const auto &entry = entry_value.as_object();

      const auto id = entry.at("id").as_string().c_str();
      std::string name{id};
      if (const auto *name_value = entry.if_contains("name");
          name_value != nullptr && name_value->is_string())
        name = name_value->as_string().c_str();

      std::string projection;
      if (const auto *projection_value = entry.if_contains("projection");
          projection_value != nullptr && projection_value->is_string())
        projection = projection_value->as_string().c_str();

      new_regions.push_back({id, std::move(name), std::move(projection)});
    }

    if (!new_regions.empty()) {
      regions = std::move(new_regions);
      regions_loaded = true;

      bool found = false;
      for (const auto &candidate : regions)
        if (candidate.id == region) {
          found = true;
          break;
        }

      if (!found)
        region = FindSkySightRegionById({}).id;
    }
  } catch (...) {
    LogError(std::current_exception(), "SkySight regions parsing failed");
    return;
  }

  owner.OnDataUpdated();
}

void
SkySightAPI::OnLayers(boost::json::value value) noexcept
{
  try {
    for (const auto &entry_value : value.as_array()) {
      const auto &entry = entry_value.as_object();
      auto *layer = GetLayer(entry.at("id").as_string().c_str());
      if (layer == nullptr)
        continue;

      if (const auto *name = entry.if_contains("name"); name != nullptr && name->is_string())
        layer->name = name->as_string().c_str();

      if (const auto *description = entry.if_contains("description");
          description != nullptr && description->is_string())
        layer->description = description->as_string().c_str();
    }

    layers_loaded = true;
  } catch (...) {
    LogError(std::current_exception(), "SkySight layers parsing failed");
    return;
  }

  owner.OnDataUpdated();
}

void
SkySightAPI::OnLastUpdates(std::string_view requested_layer_id,
                           boost::json::value value) noexcept
{
  bool active_layer_changed = false;
  bool requested_layer_found = false;

  try {
    const auto active_layer_id = owner.GetActiveLayerId();

    for (const auto &entry_value : value.as_array()) {
      const auto &entry = entry_value.as_object();
      auto *layer = GetLayer(entry.at("layer_id").as_string().c_str());
      if (layer == nullptr)
        continue;

      requested_layer_found = requested_layer_found ||
        layer->id == requested_layer_id;

      const auto update_time = ParseUpdateTime(entry.at("time"));
      if (update_time <= 0)
        continue;

      const bool changed = update_time != layer->last_update ||
        layer->live_timestamp_from_probe;
      layer->last_update = update_time;
      layer->live_timestamp_from_probe = false;
      layer->live_metadata_support = SkySight::LiveMetadataSupport::Supported;
      active_layer_changed = active_layer_changed ||
        (changed && active_layer_id == layer->id);
    }
  } catch (...) {
    LogError(std::current_exception(), "SkySight last-updated parsing failed");
    return;
  }

  if (!requested_layer_found) {
    if (auto *requested_layer = GetLayer(requested_layer_id);
        requested_layer != nullptr) {
      requested_layer->live_metadata_support =
        SkySight::LiveMetadataSupport::Unsupported;
      requested_layer->live_timestamp_from_probe = true;
    }
  }

  if (active_layer_changed)
    owner.OnDataUpdated();
}

void
SkySightAPI::OnLiveTileProbeSucceeded(std::string_view layer_id,
                                      time_t timestamp) noexcept
{
  auto *layer = GetLayer(layer_id);
  if (layer == nullptr || timestamp <= 0)
    return;

  layer->last_update = timestamp;
  layer->live_timestamp_from_probe = true;
  owner.OnDataUpdated();
}

void
SkySightAPI::OnDownloadComplete() noexcept
{
  owner.OnDataUpdated();
}
