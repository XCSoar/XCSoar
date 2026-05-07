// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightAPI.hpp"
#include "SkySightRequest.hpp"
#include "Skysight.hpp"
#include "time/Convert.hxx"
#include "util/StaticString.hxx"
#include "LogFile.hpp"

#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <ctime>

namespace {

static constexpr time_t REGIONS_RETRY_SECONDS = 30;
static constexpr time_t LAYERS_RETRY_SECONDS = 30;
static constexpr time_t INITIAL_LAST_UPDATE_POLL_SECONDS = 30;
static constexpr time_t LAST_UPDATE_POLL_SECONDS = 5 * 60;
static constexpr time_t INITIAL_DATAFILES_TIME = 0;

static time_t
ParseUpdateTime(const boost::json::value &value)
{
  if (value.is_number())
    return value.to_number<time_t>();

  if (value.is_string())
    return std::strtoll(value.as_string().c_str(), nullptr, 10);

  return 0;
}

static float
ParseFloat(const boost::json::value &value)
{
  if (value.is_number())
    return value.to_number<float>();

  if (value.is_string())
    return std::strtof(value.as_string().c_str(), nullptr);

  return 0;
}

static void
ParseLegend(const boost::json::object &entry, SkySight::Layer &layer)
{
  layer.legend.clear();

  const auto *legend_value = entry.if_contains("legend");
  if (legend_value == nullptr || !legend_value->is_object())
    return;

  const auto *colors_value = legend_value->as_object().if_contains("colors");
  if (colors_value == nullptr || !colors_value->is_array())
    return;

  for (const auto &color_value : colors_value->as_array()) {
    if (!color_value.is_object())
      continue;

    const auto &color_entry = color_value.as_object();
    const auto *value = color_entry.if_contains("value");
    const auto *color = color_entry.if_contains("color");
    if (value == nullptr || color == nullptr || !color->is_array())
      continue;

    const auto &color_array = color->as_array();
    if (color_array.size() < 3)
      continue;

    layer.legend.emplace(ParseFloat(*value),
                         SkySight::LegendColor{
                           color_array[0].to_number<uint8_t>(),
                           color_array[1].to_number<uint8_t>(),
                           color_array[2].to_number<uint8_t>(),
                         });
  }
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

  new_layers.emplace_back("satellite", "Satellite",
                          "Live SkySight satellite tiles",
                          true, true, true, 1, 8, 1.0f);

  new_layers.emplace_back("rain", "Rain",
                          "Live SkySight precipitation tiles",
                          true, true, true, 1, 8, 0.7f);
}

void
SkysightAPI::Configure(std::string_view email, std::string_view password,
                       std::string_view new_region)
{
  region = FindSkysightRegionById(new_region.empty()
                                  ? std::string_view{GetDefaultSkysightRegion().id}
                                  : new_region).id;
  ResetRegions();
  layers_loaded = false;
  last_layers_request = 0;
  ResetLastUpdates();
  selected_layers.clear();
  for (auto &layer : layers)
    layer.last_update = 0;

  request->Configure(email, password);
}

bool
SkysightAPI::HasCredentials() const noexcept
{
  return request->HasCredentials();
}

std::size_t
SkysightAPI::NumLayers() const noexcept
{
  return std::count_if(layers.begin(), layers.end(),
                       [](const auto &layer) {
                         return layer.SupportsLiveTiles();
                       });
}

const SkySight::Layer *
SkysightAPI::GetLayer(std::size_t index) const noexcept
{
  for (const auto &layer : layers)
    if (layer.SupportsLiveTiles()) {
      if (index == 0)
        return &layer;

      --index;
    }

  return nullptr;
}

SkySight::Layer *
SkysightAPI::GetLayer(std::string_view id) noexcept
{
  for (auto &i : layers)
    if (i == id)
      return &i;

  return nullptr;
}

const SkySight::Layer *
SkysightAPI::GetSelectedLayer(std::size_t index) const noexcept
{
  return index < selected_layers.size()
    ? &selected_layers[index]
    : nullptr;
}

SkySight::Layer *
SkysightAPI::GetSelectedLayer(std::string_view id) noexcept
{
  for (auto &layer : selected_layers)
    if (layer == id)
      return &layer;

  return nullptr;
}

bool
SkysightAPI::IsSelectedLayer(std::string_view id) const noexcept
{
  return std::any_of(selected_layers.begin(), selected_layers.end(),
                     [id](const auto &layer) {
                       return layer == id;
                     });
}

bool
SkysightAPI::SelectedLayersFull() const noexcept
{
  return selected_layers.size() >= MAX_SELECTED_LAYERS;
}

bool
SkysightAPI::AddSelectedLayer(const SkySight::Layer &layer)
{
  if (SelectedLayersFull() || IsSelectedLayer(layer.id))
    return false;

  selected_layers.push_back(layer);
  return true;
}

bool
SkysightAPI::RemoveSelectedLayer(std::string_view id) noexcept
{
  const auto i = std::find_if(selected_layers.begin(), selected_layers.end(),
                              [id](const auto &layer) {
                                return layer == id;
                              });
  if (i == selected_layers.end())
    return false;

  selected_layers.erase(i);
  return true;
}

void
SkysightAPI::ClearSelectedLayers() noexcept
{
  selected_layers.clear();
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
SkysightAPI::PollRegions() noexcept
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
SkysightAPI::PollSelectedDatafiles() noexcept
{
  if (!HasCredentials() || region.empty())
    return;

  for (const auto &selected : selected_layers) {
    if (!selected.updating || selected.SupportsLiveTiles())
      continue;

    request->RequestDatafiles(region, selected.id, INITIAL_DATAFILES_TIME);
    return;
  }
}

void
SkysightAPI::ResetRegions() noexcept
{
  regions = GetDefaultSkysightRegions();
  regions_loaded = false;
  last_regions_request = 0;
}

void
SkysightAPI::ResetLastUpdates() noexcept
{
  last_updates_request = 0;
}

void
SkysightAPI::OnAuthenticated() noexcept
{
  PollRegions();
  PollLayers();
  ResetLastUpdates();
  PollSelectedDatafiles();
  owner.OnDataUpdated();
}

void
SkysightAPI::OnRegions(boost::json::value value) noexcept
{
  try {
    std::vector<SkysightRegionEntry> new_regions;

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
        region = FindSkysightRegionById({}).id;
    }
  } catch (...) {
    LogError(std::current_exception(), "SkySight regions parsing failed");
    return;
  }

  owner.OnDataUpdated();
}

void
SkysightAPI::OnLayers(boost::json::value value) noexcept
{
  try {
    const auto active_layer_id = std::string{owner.GetActiveLayerId()};
    const auto displayed_layer_id = std::string{owner.GetDisplayedLayerId()};

    if (value.is_array())
      layers.reserve(layers.size() + value.as_array().size());

    for (const auto &entry_value : value.as_array()) {
      const auto &entry = entry_value.as_object();
      const auto id = entry.at("id").as_string().c_str();
      auto *layer = GetLayer(id);
      if (layer == nullptr) {
        layers.emplace_back(id, id, std::string{}, true, false, false);
        layer = &layers.back();
      }

      if (const auto *name = entry.if_contains("name");
          name != nullptr && name->is_string())
        layer->name = name->as_string().c_str();

      if (const auto *description = entry.if_contains("description");
          description != nullptr && description->is_string())
        layer->description = description->as_string().c_str();

      if (const auto *projection = entry.if_contains("projection");
          projection != nullptr && projection->is_string())
        layer->projection = projection->as_string().c_str();

      if (const auto *data_type = entry.if_contains("data_type");
          data_type != nullptr && data_type->is_string())
        layer->data_type = data_type->as_string().c_str();

      ParseLegend(entry, *layer);
      SyncSelectedLayer(layer->id);
    }

    layers_loaded = true;
    owner.OnLayerCatalogChanged(active_layer_id, displayed_layer_id);
    owner.ReloadSelectedLayersFromProfile();
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
      SyncSelectedLayer(layer->id);
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
SkysightAPI::OnDatafiles(std::string_view layer_id, boost::json::value value) noexcept
{
  auto *layer = GetLayer(layer_id);
  if (layer == nullptr)
    return;

  layer->updating = false;

  bool found = false;
  time_t first_time = 0;
  time_t last_time = 0;

  try {
    for (const auto &entry_value : value.as_array()) {
      const auto &entry = entry_value.as_object();
      const auto entry_layer_id = entry.at("layer_id").as_string().c_str();
      if (entry_layer_id != layer_id)
        continue;

      const auto update_time = ParseUpdateTime(entry.at("time"));
      if (update_time <= 0)
        continue;

      if (!found) {
        first_time = update_time;
        last_time = update_time;
        found = true;
      } else {
        first_time = std::min(first_time, update_time);
        last_time = std::max(last_time, update_time);
      }
    }
  } catch (...) {
    LogError(std::current_exception(), "SkySight datafiles parsing failed");
    OnDatafilesError(layer_id);
    return;
  }

  if (found) {
    layer->from = first_time;
    layer->to = last_time;
    layer->last_update = std::max(layer->last_update, last_time);
  }

  SyncSelectedLayer(layer_id);
  owner.OnDataUpdated();
  PollSelectedDatafiles();
}

void
SkysightAPI::OnDatafilesError(std::string_view layer_id) noexcept
{
  if (auto *layer = GetLayer(layer_id); layer != nullptr) {
    layer->updating = false;
    SyncSelectedLayer(layer_id);
  }

  owner.OnDataUpdated();
  PollSelectedDatafiles();
}

void
SkysightAPI::OnDownloadComplete() noexcept
{
  owner.OnDataUpdated();
}

void
SkysightAPI::SyncSelectedLayer(std::string_view id) noexcept
{
  auto *selected = GetSelectedLayer(id);
  auto *layer = GetLayer(id);
  if (selected == nullptr || layer == nullptr)
    return;

  *selected = *layer;
}