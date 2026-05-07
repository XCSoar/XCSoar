// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightAPI.hpp"
#include "SkySightRequest.hpp"
#include "SkySightClient.hpp"
#include "time/Convert.hxx"
#include "util/StaticString.hxx"
#include "LogFile.hpp"
#include "system/FileUtil.hpp"

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

static std::string_view
StripUrlQuery(std::string_view url) noexcept
{
  const auto split = url.find_first_of("?#");
  return split == std::string_view::npos
    ? url
    : url.substr(0, split);
}

static std::string
GetUrlSuffix(std::string_view url)
{
  const auto clean = StripUrlQuery(url);
  const auto slash = clean.find_last_of('/');
  const auto filename = slash == std::string_view::npos
    ? clean
    : clean.substr(slash + 1);
  const auto dot = filename.find_last_of('.');
  if (dot == std::string_view::npos)
    return ".zip";

  return std::string{filename.substr(dot)};
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

  new_layers.emplace_back("satellite", "Satellite",
                          "Live SkySight satellite tiles",
                          true, true, true, 1, 8, 1.0f);

  new_layers.emplace_back("rain", "Rain",
                          "Live SkySight precipitation tiles",
                          true, true, true, 1, 8, 0.7f);
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
  selected_layers.clear();
  for (auto &layer : layers)
    layer.last_update = 0;

  request->Configure(email, password);
}

bool
SkySightAPI::HasCredentials() const noexcept
{
  return request->HasCredentials();
}

std::size_t
SkySightAPI::NumLayers() const noexcept
{
  return std::count_if(layers.begin(), layers.end(),
                       [](const auto &layer) {
                         return layer.SupportsLiveTiles();
                       });
}

const SkySight::Layer *
SkySightAPI::GetLayer(std::size_t index) const noexcept
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
SkySightAPI::GetLayer(std::string_view id) noexcept
{
  for (auto &i : layers)
    if (i == id)
      return &i;

  return nullptr;
}

const SkySight::Layer *
SkySightAPI::GetSelectedLayer(std::size_t index) const noexcept
{
  return index < selected_layers.size()
    ? &selected_layers[index]
    : nullptr;
}

SkySight::Layer *
SkySightAPI::GetSelectedLayer(std::string_view id) noexcept
{
  for (auto &layer : selected_layers)
    if (layer == id)
      return &layer;

  return nullptr;
}

bool
SkySightAPI::IsSelectedLayer(std::string_view id) const noexcept
{
  return std::any_of(selected_layers.begin(), selected_layers.end(),
                     [id](const auto &layer) {
                       return layer == id;
                     });
}

bool
SkySightAPI::SelectedLayersFull() const noexcept
{
  return selected_layers.size() >= MAX_SELECTED_LAYERS;
}

bool
SkySightAPI::AddSelectedLayer(const SkySight::Layer &layer)
{
  if (SelectedLayersFull() || IsSelectedLayer(layer.id))
    return false;

  selected_layers.push_back(layer);
  return true;
}

bool
SkySightAPI::RemoveSelectedLayer(std::string_view id) noexcept
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
SkySightAPI::ClearSelectedLayers() noexcept
{
  selected_layers.clear();
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

AllocatedPath
SkySightAPI::GetDatafilePath(const SkySight::Layer &layer,
                             time_t forecast_time,
                             std::string_view suffix) const
{
  StaticString<128> filename;
  filename.Format("%s-%s-%s%s",
                  region.c_str(), layer.id.c_str(),
                  FormatFileTimestamp(forecast_time).c_str(),
                  std::string{suffix}.c_str());
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
SkySightAPI::EnsureDatafile(const SkySight::Layer &layer,
                            time_t forecast_time,
                            std::string_view link)
{
  request->DownloadDatafile(layer.id, forecast_time, link,
                            GetDatafilePath(layer, forecast_time,
                                            GetUrlSuffix(link)));
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
SkySightAPI::PollSelectedDatafiles() noexcept
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
  PollSelectedDatafiles();
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
      SyncSelectedLayer(layer->id);
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
  SyncSelectedLayer(layer_id);
  owner.OnDataUpdated();
}

void
SkySightAPI::OnDatafiles(std::string_view layer_id, boost::json::value value) noexcept
{
  auto *layer = GetLayer(layer_id);
  if (layer == nullptr)
    return;

  layer->updating = false;

  bool found = false;
  time_t first_time = 0;
  time_t last_time = 0;
  time_t newest_time = 0;
  std::string newest_link;

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

      if (const auto *link = entry.if_contains("link");
          link != nullptr && link->is_string() && update_time >= newest_time) {
        newest_time = update_time;
        newest_link = link->as_string().c_str();
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

    if (!newest_link.empty())
      EnsureDatafile(*layer, newest_time, newest_link);
  }

  SyncSelectedLayer(layer_id);
  owner.OnDataUpdated();
  PollSelectedDatafiles();
}

void
SkySightAPI::OnDatafilesError(std::string_view layer_id) noexcept
{
  if (auto *layer = GetLayer(layer_id); layer != nullptr) {
    layer->updating = false;
    SyncSelectedLayer(layer_id);
  }

  owner.OnDataUpdated();
  PollSelectedDatafiles();
}

void
SkySightAPI::OnDownloadComplete() noexcept
{
  owner.OnDataUpdated();
}

void
SkySightAPI::OnDatafileDownloaded(std::string_view layer_id,
                                  time_t forecast_time,
                                  Path path) noexcept
{
  auto *layer = GetLayer(layer_id);
  if (layer == nullptr)
    return;

  layer->last_update = std::max(layer->last_update, forecast_time);
  layer->mtime = std::chrono::system_clock::to_time_t(
    File::GetLastModification(path));
  SyncSelectedLayer(layer_id);
  owner.OnDataUpdated();
}

void
SkySightAPI::OnDatafileError(std::string_view layer_id,
                             [[maybe_unused]] time_t forecast_time) noexcept
{
  SyncSelectedLayer(layer_id);
  owner.OnDataUpdated();
}

void
SkySightAPI::SyncSelectedLayer(std::string_view id) noexcept
{
  auto *selected = GetSelectedLayer(id);
  auto *layer = GetLayer(id);
  if (selected == nullptr || layer == nullptr)
    return;

  *selected = *layer;
}
