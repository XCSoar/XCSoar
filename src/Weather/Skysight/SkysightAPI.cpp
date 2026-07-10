// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkysightAPI.hpp"
#include "ForecastUtils.hpp"
#include "SkySightFileDecoder.hpp"
#include "SkySightRequest.hpp"
#include "SkySightURL.hpp"
#include "Skysight.hpp"
#include "io/FileLineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "json/Serialize.hxx"
#include "time/Convert.hxx"
#include "util/StaticString.hxx"
#include "util/UTF8.hpp"
#include "LogFile.hpp"
#include "system/FileUtil.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>

namespace {

static constexpr time_t REGIONS_RETRY_SECONDS = 30;
static constexpr time_t LAYERS_RETRY_SECONDS = 30;
static constexpr time_t CATALOG_CACHE_MAX_AGE = 12 * 60 * 60;
static constexpr time_t INITIAL_LAST_UPDATE_POLL_SECONDS = 30;
static constexpr time_t LAST_UPDATE_POLL_SECONDS = 5 * 60;
static constexpr const char *REGIONS_CACHE_FILE = "regions-v1.cache";

static time_t
GetInitialDatafilesTime() noexcept
{
  return SkySight::GetForecastDayStart(std::time(nullptr));
}

struct ForecastDatafileChoice {
  time_t time = 0;
  std::string link;
};

[[nodiscard]] static ForecastDatafileChoice
ChooseDefaultForecastDatafile(time_t latest_past_time,
                              std::string latest_past_link,
                              time_t earliest_future_time,
                              std::string earliest_future_link) noexcept
{
  if (latest_past_time > 0)
    return {latest_past_time, std::move(latest_past_link)};

  return {earliest_future_time, std::move(earliest_future_link)};
}

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

  if (filename.ends_with(".min")) {
    const auto inner = filename.substr(0, filename.size() - 4);
    const auto inner_dot = inner.find_last_of('.');
    if (inner_dot != std::string_view::npos)
      return std::string{filename.substr(inner_dot)};
  }

  const auto dot = filename.find_last_of('.');
  if (dot == std::string_view::npos)
    return ".zip";

  return std::string{filename.substr(dot)};
}

[[nodiscard]] static bool
NeedsNetCdfDecodeSuffix(std::string_view suffix) noexcept
{
  return suffix == ".nc" || suffix == ".nc.min" || suffix == ".min";
}

[[nodiscard]] static bool
MetadataCacheFresh(time_t last_refresh, time_t now) noexcept
{
  return last_refresh != 0 && now < last_refresh + CATALOG_CACHE_MAX_AGE;
}

static time_t
GetMetadataCacheTime(Path path) noexcept
{
  const auto modification = File::GetLastModification(path);
  return modification == std::chrono::system_clock::time_point{}
    ? 0
    : std::chrono::system_clock::to_time_t(modification);
}

static bool
LoadMetadataCache(Path path, boost::json::value &value) noexcept
{
  std::string json;

  try {
    FileLineReaderA reader(path);
    const char *line;
    while ((line = reader.ReadLine()) != nullptr) {
      json += line;
      json += '\n';
    }
  } catch (...) {
    return false;
  }

  if (json.empty())
    return false;

  try {
    value = boost::json::parse(json);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace

SkysightAPI::SkysightAPI(Skysight &_owner, CurlGlobal &curl, Path _cache_path)
  :owner(_owner),
   request(std::make_unique<SkySightRequest>(*this, curl, _cache_path)),
   decode_job(std::make_unique<SkySightFileDecodeJob>()),
   cache_path(_cache_path)
{
  InitialiseLayers(layers);
}

SkysightAPI::~SkysightAPI() = default;

void
SkysightAPI::UpdateBusyState(SkySight::Layer &layer) noexcept
{
  layer.updating = layer.ShouldShowUpdating();
}

AllocatedPath
SkysightAPI::GetRegionsCachePath() const noexcept
{
  return AllocatedPath::Build(cache_path, REGIONS_CACHE_FILE);
}

AllocatedPath
SkysightAPI::GetLayersCachePath() const noexcept
{
  StaticString<128> filename;
  filename.Format("layers-%s-v1.cache", region.c_str());
  return AllocatedPath::Build(cache_path, filename.c_str());
}

void
SkysightAPI::LoadCachedRegions() noexcept
{
  boost::json::value value;
  const auto path = GetRegionsCachePath();
  if (!LoadMetadataCache(path, value) ||
      !ParseRegions(value, "SkySight cached regions parsing failed"))
    return;

  last_regions_refresh = GetMetadataCacheTime(path);
}

void
SkysightAPI::LoadCachedLayers() noexcept
{
  boost::json::value value;
  const auto path = GetLayersCachePath();
  if (!LoadMetadataCache(path, value) ||
      !ParseLayers(value, "SkySight cached layers parsing failed"))
    return;

  last_layers_refresh = GetMetadataCacheTime(path);
}

void
SkysightAPI::StoreMetadataCache(Path path,
                                const boost::json::value &value) const noexcept
{
  try {
    Directory::Create(cache_path);
    FileOutputStream file(path);
    Json::Serialize(file, value);
    file.Commit();
  } catch (...) {
    LogError(std::current_exception(), "SkySight metadata cache write failed");
  }
}

bool
SkysightAPI::ParseRegions(const boost::json::value &value,
                          const char *error_context) noexcept
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

    return true;
  } catch (...) {
    LogError(std::current_exception(), error_context);
    return false;
  }
}

bool
SkysightAPI::ParseLayers(const boost::json::value &value,
                         const char *error_context) noexcept
{
  try {
    std::vector<SkySight::Layer> new_layers;
    InitialiseLayers(new_layers);

    auto *const old_layers = &layers;
    const auto find_old_layer = [old_layers](std::string_view id) noexcept
      -> const SkySight::Layer * {
      for (const auto &layer : *old_layers)
        if (layer == id)
          return &layer;

      return nullptr;
    };

    const auto copy_runtime_state = [&find_old_layer](SkySight::Layer &layer) {
      const auto *existing = find_old_layer(layer.id);
      if (existing == nullptr)
        return;

      layer.time_name = existing->time_name;
      layer.forecast_datafiles = existing->forecast_datafiles;
      layer.from = existing->from;
      layer.to = existing->to;
      layer.mtime = existing->mtime;
      layer.updating = existing->updating;
      layer.datafiles_pending = existing->datafiles_pending;
      layer.preload_requested = existing->preload_requested;
      layer.pending_downloads = existing->pending_downloads;
      layer.last_update = existing->last_update;
      layer.forecast_time = existing->forecast_time;
    };

    for (auto &layer : new_layers)
      copy_runtime_state(layer);

    if (value.is_array())
      new_layers.reserve(new_layers.size() + value.as_array().size());

    const auto find_new_layer = [&new_layers](std::string_view id) noexcept
      -> SkySight::Layer * {
      for (auto &layer : new_layers)
        if (layer == id)
          return &layer;

      return nullptr;
    };

    for (const auto &entry_value : value.as_array()) {
      const auto &entry = entry_value.as_object();
      const auto id = entry.at("id").as_string().c_str();
      auto *layer = find_new_layer(id);
      if (layer == nullptr) {
        new_layers.emplace_back(id, id, std::string{}, true, false, false);
        layer = &new_layers.back();
        copy_runtime_state(*layer);
      }

      if (const auto *name = entry.if_contains("name");
          name != nullptr && name->is_string()) {
        const std::string_view text{name->as_string().c_str(),
                                    name->as_string().size()};
        layer->name = ValidateUTF8(text) ? std::string{text} : layer->id;
      }

      if (const auto *description = entry.if_contains("description");
          description != nullptr && description->is_string()) {
        const std::string_view text{description->as_string().c_str(),
                                    description->as_string().size()};
        layer->description = ValidateUTF8(text)
          ? std::string{text}
          : std::string{};
      }

      if (const auto *projection = entry.if_contains("projection");
          projection != nullptr && projection->is_string())
        layer->projection = projection->as_string().c_str();

      if (const auto *data_type = entry.if_contains("data_type");
          data_type != nullptr && data_type->is_string())
        layer->data_type = data_type->as_string().c_str();

      ParseLegend(entry, *layer);
    }

    layers = std::move(new_layers);
    layers_loaded = true;
    return true;
  } catch (...) {
    LogError(std::current_exception(), error_context);
    return false;
  }
}

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
  if (decode_job != nullptr)
    decode_job->Cancel();
  pending_decode_jobs.clear();
  InitialiseLayers(layers);
  selected_layers.clear();

  request->Configure(email, password);
  LoadCachedRegions();
  LoadCachedLayers();
}

bool
SkysightAPI::HasCredentials() const noexcept
{
  return request->HasCredentials();
}

bool
SkysightAPI::IsThrottled() const noexcept
{
  return request->IsThrottled();
}

time_t
SkysightAPI::GetThrottleRemainingSeconds() const noexcept
{
  return request->GetThrottleRemainingSeconds();
}

std::size_t
SkysightAPI::NumLayers() const noexcept
{
  return layers.size();
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
  return SkySightUrl::Tile(layer.id, tile.zoom, tile.x, tile.y,
                           FormatUrlTimestamp(timestamp));
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

AllocatedPath
SkysightAPI::GetDatafilePath(const SkySight::Layer &layer,
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
SkysightAPI::EnsureTile(const SkySight::Layer &layer, time_t timestamp,
                        const GeoBitmap::TileData &tile)
{
  request->DownloadFile(MakeTileUrl(layer, timestamp, tile),
                        GetTilePath(layer, timestamp, tile),
                        layer.requires_auth);
}

void
SkysightAPI::CancelTileDownloads() noexcept
{
  request->CancelTileDownloads();
}

void
SkysightAPI::EnsureDatafile(const SkySight::Layer &layer,
                            time_t forecast_time,
                            std::string_view link)
{
  request->DownloadDatafile(layer.id, forecast_time, link,
                            GetDatafilePath(layer, forecast_time,
                                            GetUrlSuffix(link)));
}

bool
SkysightAPI::QueueForecastDatafile(SkySight::Layer &layer,
                                   time_t forecast_time,
                                   std::string_view link) noexcept
{
  if (link.empty())
    return false;

  const auto suffix = GetUrlSuffix(link);
  if (!SkySightFileDecoder::IsNetCdfDecodeAvailable() &&
      NeedsNetCdfDecodeSuffix(suffix))
    return false;

  switch (request->DownloadDatafile(layer.id, forecast_time, link,
                                    GetDatafilePath(layer, forecast_time, suffix))) {
  case SkySightRequest::DownloadDatafileResult::Duplicate:
  case SkySightRequest::DownloadDatafileResult::Available:
    return true;

  case SkySightRequest::DownloadDatafileResult::Queued:
    ++layer.pending_downloads;
    UpdateBusyState(layer);
    return true;
  }

  return false;
}

void
SkysightAPI::QueueDecodeJob(Path path, const SkySight::Layer &layer,
                            time_t forecast_time) noexcept
{
  pending_decode_jobs.push_back(PendingDecodeJob{
    SkySightPreparedData{
      SkySightPreparedDataKind::NeedsNetCdfDecode,
      AllocatedPath(path.c_str()),
      path.WithSuffix(".tif"),
    },
    std::string{layer.id},
    layer.legend,
    std::string{layer.id},
    forecast_time,
  });
}

void
SkysightAPI::StartNextDecodeJob() noexcept
{
  if (pending_decode_jobs.empty())
    return;

  if (decode_job == nullptr)
    decode_job = std::make_unique<SkySightFileDecodeJob>();

  if (decode_job->GetStatus() != SkySightFileDecodeJob::Status::Idle)
    return;

  auto job = std::move(pending_decode_jobs.front());
  pending_decode_jobs.pop_front();

  decode_job->Start(
    std::move(job.prepared),
    std::move(job.variable_name),
    std::move(job.legend),
    [this, layer_id = std::move(job.layer_id), forecast_time = job.forecast_time](AllocatedPath output_path) {
      OnDatafileDownloaded(layer_id, forecast_time, output_path);
    },
    [this, layer_id = std::move(job.layer_id), forecast_time = job.forecast_time](std::exception_ptr error) {
      LogError(error, "SkySight forecast decode failed");
      OnDatafileError(layer_id, forecast_time);
    });
}

bool
SkysightAPI::QueueForecastDatafile(std::string_view layer_id,
                                   time_t forecast_time,
                                   std::string_view link) noexcept
{
  auto *layer = GetLayer(layer_id);
  if (layer == nullptr)
    return false;

  const bool success = QueueForecastDatafile(*layer, forecast_time, link);
  SyncSelectedLayer(layer_id);
  return success;
}

bool
SkysightAPI::PreloadDatafiles(std::string_view layer_id) noexcept
{
  auto *layer = GetLayer(layer_id);
  if (layer == nullptr || layer->SupportsLiveTiles())
    return false;

  bool success = false;

  if (layer->forecast_datafiles.empty()) {
    layer->preload_requested = true;
    layer->datafiles_pending = true;
    UpdateBusyState(*layer);
    success = true;
  } else {
    layer->preload_requested = false;
    for (const auto *datafile :
         SkySight::GetForecastPreloadDatafiles(*layer, std::time(nullptr)))
      success = QueueForecastDatafile(*layer, datafile->time, datafile->link) ||
        success;

    UpdateBusyState(*layer);
  }

  SyncSelectedLayer(layer_id);
  owner.OnDataUpdated();

  if (layer->datafiles_pending)
    PollSelectedDatafiles();

  return success;
}

bool
SkysightAPI::PreloadAllDatafiles() noexcept
{
  bool success = false;

  for (const auto &selected : selected_layers)
    if (!selected.SupportsLiveTiles())
      success = PreloadDatafiles(selected.id) || success;

  return success;
}

void
SkysightAPI::PollRegions() noexcept
{
  if (!HasCredentials())
    return;

  const auto now = std::time(nullptr);
  if (regions_loaded && MetadataCacheFresh(last_regions_refresh, now))
    return;

  if (last_regions_request != 0 && now < last_regions_request + REGIONS_RETRY_SECONDS)
    return;

  try {
    if (request->RequestRegions())
      last_regions_request = now;
  } catch (...) {
    LogError(std::current_exception(), "SkySight regions polling failed");
  }
}

void
SkysightAPI::PollLayers() noexcept
{
  if (!HasCredentials() || region.empty())
    return;

  const auto now = std::time(nullptr);
  if (layers_loaded && MetadataCacheFresh(last_layers_refresh, now))
    return;

  if (last_layers_request != 0 && now < last_layers_request + LAYERS_RETRY_SECONDS)
    return;

  try {
    if (request->RequestLayers(region))
      last_layers_request = now;
  } catch (...) {
    LogError(std::current_exception(), "SkySight layers polling failed");
  }
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

  try {
    if (request->RequestLastUpdates(region))
      last_updates_request = now;
  } catch (...) {
    LogError(std::current_exception(), "SkySight last-updated polling failed");
  }
}

void
SkysightAPI::PollSelectedDatafiles() noexcept
{
  if (!HasCredentials() || region.empty())
    return;

  const auto request_pending = [this](const SkySight::Layer &layer) {
    request->RequestDatafiles(region, layer.id, GetInitialDatafilesTime());
  };

  const auto is_pending_forecast = [](const SkySight::Layer &layer) noexcept {
    return layer.datafiles_pending && !layer.SupportsLiveTiles();
  };

  if (const auto active_layer_id = owner.GetActiveLayerId(); !active_layer_id.empty()) {
    if (const auto active = std::find_if(selected_layers.begin(), selected_layers.end(),
                                         [active_layer_id, &is_pending_forecast](const auto &layer) {
                                           return layer.id == active_layer_id &&
                                             is_pending_forecast(layer);
                                         });
        active != selected_layers.end()) {
      request_pending(*active);
      return;
    }
  }

  if (const auto preload = std::find_if(selected_layers.begin(), selected_layers.end(),
                                        [&is_pending_forecast](const auto &layer) {
                                          return layer.preload_requested &&
                                            is_pending_forecast(layer);
                                        });
      preload != selected_layers.end()) {
    request_pending(*preload);
    return;
  }

  if (const auto selected = std::find_if(selected_layers.begin(), selected_layers.end(),
                                         [&is_pending_forecast](const auto &layer) {
                                           return is_pending_forecast(layer);
                                         });
      selected != selected_layers.end())
    request_pending(*selected);
}

void
SkysightAPI::ResetRegions() noexcept
{
  regions = GetDefaultSkysightRegions();
  regions_loaded = false;
  last_regions_request = 0;
  last_regions_refresh = 0;
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
  if (!ParseRegions(value, "SkySight regions parsing failed"))
    return;

  last_regions_refresh = std::time(nullptr);
  StoreMetadataCache(GetRegionsCachePath(), value);

  owner.OnDataUpdated();
}

void
SkysightAPI::OnLayers(boost::json::value value) noexcept
{
  try {
    const auto active_layer_id = std::string{owner.GetActiveLayerId()};
    const auto displayed_layer_id = std::string{owner.GetDisplayedLayerId()};

    if (!ParseLayers(value, "SkySight layers parsing failed"))
      return;

    last_layers_refresh = std::time(nullptr);
    StoreMetadataCache(GetLayersCachePath(), value);
    owner.OnLayerCatalogChanged(active_layer_id, displayed_layer_id);
    owner.ReloadSelectedLayersFromProfile();

    owner.OnDataUpdated();
  } catch (...) {
    LogError(std::current_exception(), "SkySight layer catalog update failed");
  }
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

  layer->datafiles_pending = false;
  layer->forecast_datafiles.clear();

  bool found = false;
  time_t first_time = 0;
  time_t last_time = 0;
  time_t latest_past_time = 0;
  std::string latest_past_link;
  time_t earliest_future_time = 0;
  std::string earliest_future_link;
  const auto now = std::time(nullptr);
  std::vector<SkySight::ForecastDatafile> forecast_datafiles;

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
          link != nullptr && link->is_string()) {
        const std::string link_value{link->as_string().c_str()};
        if (const auto existing = std::find_if(forecast_datafiles.begin(),
                                               forecast_datafiles.end(),
                                               [update_time](const auto &candidate) {
                                                 return candidate.time == update_time;
                                               });
            existing == forecast_datafiles.end())
          forecast_datafiles.emplace_back(update_time, link_value);
        else
          existing->link = link_value;

        if (update_time <= now) {
          if (latest_past_time == 0 || update_time > latest_past_time) {
            latest_past_time = update_time;
            latest_past_link = link_value;
          }
        } else if (earliest_future_time == 0 || update_time < earliest_future_time) {
          earliest_future_time = update_time;
          earliest_future_link = link_value;
        }
      }
    }
  } catch (...) {
    LogError(std::current_exception(), "SkySight datafiles parsing failed");
    OnDatafilesError(layer_id);
    return;
  }

  if (found) {
    std::sort(forecast_datafiles.begin(), forecast_datafiles.end(),
              [](const auto &a, const auto &b) {
                return a.time > b.time;
              });
    layer->forecast_datafiles = std::move(forecast_datafiles);
    layer->from = first_time;
    layer->to = last_time;
    layer->last_update = std::max(layer->last_update, last_time);

    const auto default_choice = ChooseDefaultForecastDatafile(latest_past_time,
                                                              std::move(latest_past_link),
                                                              earliest_future_time,
                                                              std::move(earliest_future_link));

    if (layer->forecast_time == 0 ||
        std::none_of(layer->forecast_datafiles.begin(),
                     layer->forecast_datafiles.end(),
                     [layer](const auto &candidate) {
                       return candidate.time == layer->forecast_time;
                     }))
      layer->forecast_time = default_choice.time;

    const auto selected = std::find_if(layer->forecast_datafiles.begin(),
                                       layer->forecast_datafiles.end(),
                                       [layer](const auto &candidate) {
                                         return candidate.time == layer->forecast_time;
                                       });

    const bool preload_requested = std::exchange(layer->preload_requested, false);
    if (preload_requested) {
      for (const auto *datafile :
           SkySight::GetForecastPreloadDatafiles(*layer, now))
        (void)QueueForecastDatafile(*layer, datafile->time, datafile->link);
    } else if (owner.GetActiveLayerId() == layer_id &&
               selected != layer->forecast_datafiles.end()) {
      (void)QueueForecastDatafile(*layer, selected->time, selected->link);
    }
  } else {
    layer->preload_requested = false;
    layer->forecast_time = 0;
  }

  UpdateBusyState(*layer);
  SyncSelectedLayer(layer_id);
  owner.OnDataUpdated();
  PollSelectedDatafiles();
}

void
SkysightAPI::OnDatafilesError(std::string_view layer_id) noexcept
{
  if (auto *layer = GetLayer(layer_id); layer != nullptr) {
    layer->datafiles_pending = false;
    layer->preload_requested = false;
    UpdateBusyState(*layer);
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
SkysightAPI::OnDatafileDownloaded(std::string_view layer_id,
                                  time_t forecast_time,
                                  Path path) noexcept
{
  auto *layer = GetLayer(layer_id);
  if (layer == nullptr)
    return;

  if (path.EndsWithIgnoreCase(".nc")) {
    UpdateBusyState(*layer);
    SyncSelectedLayer(layer_id);
    owner.OnDataUpdated();

    QueueDecodeJob(path, *layer, forecast_time);
    StartNextDecodeJob();
    return;
  }

  if (layer->pending_downloads > 0)
    --layer->pending_downloads;

  UpdateBusyState(*layer);
  layer->last_update = std::max(layer->last_update, forecast_time);
  layer->mtime = std::chrono::system_clock::to_time_t(
    File::GetLastModification(path));
  SyncSelectedLayer(layer_id);
  owner.OnDataUpdated();
  StartNextDecodeJob();
}

void
SkysightAPI::OnDatafileError(std::string_view layer_id,
                             [[maybe_unused]] time_t forecast_time,
                             [[maybe_unused]] bool preparation_failed) noexcept
{
  if (auto *layer = GetLayer(layer_id); layer != nullptr) {
    if (layer->pending_downloads > 0)
      --layer->pending_downloads;

    UpdateBusyState(*layer);
  }

  SyncSelectedLayer(layer_id);
  owner.OnDataUpdated();
  StartNextDecodeJob();
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
