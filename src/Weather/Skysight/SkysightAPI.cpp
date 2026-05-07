// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightAPI.hpp"
#include "SkySightRequest.hpp"
#include "Skysight.hpp"
#include "time/Convert.hxx"
#include "util/StaticString.hxx"
#include "LogFile.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>

namespace {

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

SkysightAPI::SkysightAPI(Skysight &_owner, CurlGlobal &curl, Path _cache_path)
  :owner(_owner),
   request(std::make_unique<SkySightRequest>(*this, curl)),
   cache_path(_cache_path)
{
  InitialiseLayers(layers);
}

SkysightAPI::~SkysightAPI() = default;

void
SkysightAPI::InitialiseLayers(std::vector<SkySight::Layer> &new_layers)
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
SkysightAPI::Configure(std::string_view email, std::string_view password,
                       std::string_view new_region)
{
  region = FindSkysightRegionById(new_region.empty()
                                  ? std::string_view{GetDefaultSkysightRegion().id}
                                  : new_region).id;
  layers_loaded = false;
  last_layers_request = 0;
  ResetLastUpdates();
  for (auto &layer : layers)
    layer.last_update = 0;

  request->Configure(email, password);
}

bool
SkysightAPI::HasCredentials() const noexcept
{
  return request->HasCredentials();
}

const SkySight::Layer *
SkysightAPI::GetLayer(std::size_t index) const noexcept
{
  return index < layers.size()
    ? &layers[index]
    : nullptr;
}

SkySight::Layer *
SkysightAPI::GetLayer(std::string_view id) noexcept
{
  for (auto &i : layers)
    if (i == id)
      return &i;

  return nullptr;
}

std::string
SkysightAPI::FormatUrlTimestamp(time_t timestamp)
{
  const auto tm = GmTime(std::chrono::system_clock::from_time_t(timestamp));

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y/%m/%d/%H%M", &tm);
  return buffer;
}

std::string
SkysightAPI::FormatFileTimestamp(time_t timestamp)
{
  const auto tm = GmTime(std::chrono::system_clock::from_time_t(timestamp));

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H%M", &tm);
  return buffer;
}

std::string
SkysightAPI::MakeTileUrl(const SkySight::Layer &layer,
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
SkysightAPI::GetTilePath(const SkySight::Layer &layer, time_t timestamp,
                         const GeoBitmap::TileData &tile) const
{
  StaticString<128> filename;
  filename.Format("%s-%u-%u-%u-%s.jpg",
                  layer.id.c_str(), tile.zoom, tile.x, tile.y,
                  FormatFileTimestamp(timestamp).c_str());
  return AllocatedPath::Build(cache_path, filename);
}

void
SkysightAPI::EnsureTile(const SkySight::Layer &layer, time_t timestamp,
                        const GeoBitmap::TileData &tile)
{
  request->DownloadFile(MakeTileUrl(layer, timestamp, tile),
                        GetTilePath(layer, timestamp, tile),
                        layer.requires_auth);
}

void
SkysightAPI::PollLayers() noexcept
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
SkysightAPI::PollLastUpdates() noexcept
{
  if (!HasCredentials() || region.empty())
    return;

  const auto now = std::time(nullptr);
  const auto interval = owner.GetActiveLayerId().empty() ||
      GetLayer(owner.GetActiveLayerId()) == nullptr ||
      GetLayer(owner.GetActiveLayerId())->last_update != 0
    ? LAST_UPDATE_POLL_SECONDS
    : INITIAL_LAST_UPDATE_POLL_SECONDS;

  if (last_updates_request != 0 && now < last_updates_request + interval)
    return;

  last_updates_request = now;
  request->RequestLastUpdates(region);
}

void
SkysightAPI::ResetLastUpdates() noexcept
{
  last_updates_request = 0;
}

void
SkysightAPI::OnAuthenticated() noexcept
{
  PollLayers();
  ResetLastUpdates();
  owner.OnDataUpdated();
}

void
SkysightAPI::OnLayers(boost::json::value value) noexcept
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
SkysightAPI::OnLastUpdates(boost::json::value value) noexcept
{
  bool active_layer_changed = false;

  try {
    const auto active_layer_id = owner.GetActiveLayerId();

    for (const auto &entry_value : value.as_array()) {
      const auto &entry = entry_value.as_object();
      auto *layer = GetLayer(entry.at("layer_id").as_string().c_str());
      if (layer == nullptr)
        continue;

      const auto update_time = ParseUpdateTime(entry.at("time"));
      if (update_time <= 0 || update_time == layer->last_update)
        continue;

      layer->last_update = update_time;
      active_layer_changed = active_layer_changed || active_layer_id == layer->id;
    }
  } catch (...) {
    LogError(std::current_exception(), "SkySight last-updated parsing failed");
    return;
  }

  if (active_layer_changed)
    owner.OnDataUpdated();
}

void
SkysightAPI::OnDownloadComplete() noexcept
{
  owner.OnDataUpdated();
}