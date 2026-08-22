// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightClient.hpp"
#include "SkySightCache.hpp"
#include "SkySightAPI.hpp"
#include "ForecastUtils.hpp"
#include "LiveTileUtils.hpp"
#include "RegionTime.hpp"
#include "SkySightFileDecoder.hpp"
#include "Weather/BackgroundDownloadProgress.hpp"
#include "Weather/MapOverlay/PagePlacement.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "Message.hpp"
#include "PageActions.hpp"
#include "Profile/Current.hpp"
#include "Profile/PageProfile.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "MainWindow.hpp"
#include "UIGlobals.hpp"
#include "LocalPath.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "MapWindow/OverlayBitmap.hpp"
#include "system/FileUtil.hpp"
#include "thread/Debug.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <exception>
#include <map>
#include <set>
#include <tuple>

namespace {

static constexpr auto CLEANUP_CHECK_INTERVAL = std::chrono::minutes{1};

void
MigrateCacheFiles(Path source_path, Path destination_path) noexcept
{
  if (!Directory::Exists(source_path))
    return;

  struct Visitor final : File::Visitor {
    const Path destination_path;

    explicit Visitor(Path _destination_path) noexcept
      :destination_path(_destination_path) {}

    void Visit(Path source_path, Path filename) override {
      const auto target_path = AllocatedPath::Build(destination_path, filename);
      if (File::ExistsAny(target_path))
        File::Delete(source_path);
      else
        File::Rename(source_path, target_path);
    }
  } visitor{destination_path};

  try {
    Directory::VisitFiles(source_path, visitor);
  } catch (...) {
    LogError(std::current_exception(), "SkySight cache migration failed");
    return;
  }

  struct EmptyVisitor final : Directory::DirEntryVisitor {
    bool empty = true;

    void Visit(Path, Path, bool) noexcept override {
      empty = false;
    }
  } empty_visitor;

  Directory::VisitDirectoriesAndFiles(source_path, empty_visitor);
  if (empty_visitor.empty)
    (void)Directory::Remove(source_path);
}

[[nodiscard]] static bool
HasExactForecastImage(std::string_view region,
                      const SkySight::Layer &layer)
{
  if (layer.forecast_time <= 0)
    return false;

  const auto candidate = SkySightCache::FindForecastImage(
    SkySightClient::GetCachePath(), region, layer.id, layer.forecast_time);
  return candidate.path != nullptr &&
    candidate.forecast_time == layer.forecast_time &&
    File::Exists(candidate.path);
}

[[nodiscard]] bool
SyncCachedForecastImage(std::string_view region,
                        SkySight::Layer &layer,
                        time_t forecast_time)
{
  const auto candidate = SkySightCache::FindForecastImage(
    SkySightClient::GetCachePath(), region, layer.id, forecast_time);
  if (candidate.path == nullptr || candidate.forecast_time != forecast_time)
    return false;

  const auto mtime = std::chrono::system_clock::to_time_t(
    File::GetLastModification(candidate.path));
  layer.mtime = mtime;
  return true;
}

#ifdef ENABLE_OPENGL

struct PrioritizedTile {
  GeoBitmap::TileData tile;
  unsigned priority;
};

struct CachedLiveTile {
  AllocatedPath path;
  time_t timestamp = 0;
  bool exists = false;
};

class LiveTileCacheIndex {
  using Key = std::tuple<time_t, unsigned, uint16_t, uint16_t>;

  SkySightAPI &api;
  const SkySight::Layer &layer;
  std::map<Key, CachedLiveTile> entries;

public:
  LiveTileCacheIndex(SkySightAPI &_api,
                     const SkySight::Layer &_layer) noexcept
    :api(_api), layer(_layer) {}

  [[nodiscard]] const CachedLiveTile &
  Find(const GeoBitmap::TileData &tile, time_t timestamp)
  {
    const Key key{timestamp, tile.zoom, tile.x, tile.y};
    auto [it, inserted] = entries.try_emplace(key);
    if (inserted) {
      it->second.path = api.GetTilePath(layer, timestamp, tile);
      it->second.timestamp = timestamp;
      it->second.exists = File::Exists(it->second.path);
    }

    return it->second;
  }
};

struct TargetLiveTile {
  GeoBitmap::TileData tile;
  bool exact_refresh_available;
};

struct DisplayLiveTile {
  GeoBitmap::TileData tile;
  time_t timestamp;
  std::string path;
};

void
AppendUniqueLiveTile(std::vector<DisplayLiveTile> &items,
                     const DisplayLiveTile &candidate)
{
  if (std::none_of(items.begin(), items.end(),
                   [&candidate](const auto &item) {
                     return SkySight::IsSameTile(item.tile, candidate.tile) &&
                       item.path == candidate.path;
                   }))
    items.push_back(candidate);
}

[[nodiscard]] std::vector<GeoBitmap::TileData>
CollectVisibleLiveTiles(const GeoBounds &map_bounds,
                        const GeoBitmap::TileData &base_tile,
                        const GeoBounds &region_bounds,
                        unsigned range)
{
  const int tiles_per_axis = 1 << base_tile.zoom;
  const auto normalize_x = [tiles_per_axis](int value) {
    int result = value % tiles_per_axis;
    if (result < 0)
      result += tiles_per_axis;

    return (uint16_t)result;
  };

  std::vector<PrioritizedTile> candidates;
  const auto diameter = 2 * range + 1;
  candidates.reserve(diameter * diameter);
  const int tile_range = int(range);
  for (int dx = -tile_range; dx <= tile_range; ++dx) {
    for (int dy = -tile_range; dy <= tile_range; ++dy) {
      const int y = int(base_tile.y) + dy;
      if (y < 0 || y >= tiles_per_axis)
        continue;

      const GeoBitmap::TileData tile{
        base_tile.zoom,
        normalize_x(int(base_tile.x) + dx),
        (uint16_t)y,
      };
      const auto tile_bounds = GeoBitmap::GetBounds(tile);
      if (!tile_bounds.Overlaps(map_bounds) ||
          (region_bounds.IsValid() && !tile_bounds.Overlaps(region_bounds)))
        continue;

      candidates.push_back({tile, unsigned(dx * dx + dy * dy)});
    }
  }

  std::stable_sort(candidates.begin(), candidates.end(),
                   [](const auto &a, const auto &b) {
                     return a.priority < b.priority;
                   });

  std::vector<GeoBitmap::TileData> result;
  result.reserve(candidates.size());
  for (const auto &candidate : candidates)
    result.push_back(candidate.tile);

  return result;
}

[[nodiscard]] bool
IsSameTileSequence(const std::vector<GeoBitmap::TileData> &a,
                   const std::vector<GeoBitmap::TileData> &b) noexcept
{
  return a.size() == b.size() &&
    std::equal(a.begin(), a.end(), b.begin(), SkySight::IsSameTile);
}

[[nodiscard]] constexpr bool
IsSameBounds(const GeoBounds &a, const GeoBounds &b) noexcept
{
  return a.GetWest() == b.GetWest() && a.GetEast() == b.GetEast() &&
    a.GetSouth() == b.GetSouth() && a.GetNorth() == b.GetNorth();
}

#endif

} // namespace
SkySightClient::SkySightClient(CurlGlobal &curl)
  :api(std::make_unique<SkySightAPI>(*this, curl, GetCachePath())),
   request_timer([this]{
     MaybeCleanupFiles();
     api->Poll();
   })
{
  Init();
}

SkySightClient::~SkySightClient()
{
  BeginShutdown();
}

void
SkySightClient::BeginShutdown() noexcept
{
  request_timer.Cancel();
  if (api != nullptr)
    api->BeginShutdown();
}

AllocatedPath
SkySightClient::GetCachePath() noexcept
{
  const auto weather_path = MakeCacheDirectory("weather");
  auto skysight_path = AllocatedPath::Build(weather_path, "skysight");
  try {
    Directory::Create(skysight_path);
  } catch (...) {
    LogError(std::current_exception(), "SkySight cache directory creation failed");
  }
  return skysight_path;
}

void
SkySightClient::Init()
{
  request_timer.Cancel();

  const auto cache_path = GetCachePath();
  MigrateCacheFiles(AllocatedPath::Build(::GetCachePath(), "skysight"),
                    cache_path);
  MigrateCacheFiles(AllocatedPath::Build(LocalPath("weather"), "skysight"),
                    cache_path);
  forecast_cleanup_pending = !CleanupFiles();
  next_cleanup_check = std::chrono::steady_clock::now() +
    CLEANUP_CHECK_INTERVAL;

  ResetTiles();
  active_layer = nullptr;

  const auto &settings = CommonInterface::GetComputerSettings().weather.skysight;
  api->Configure(settings.email.c_str(), settings.password.c_str(),
                 settings.region.c_str());
  ReloadSelectedLayersFromProfile();
  api->PollRegions();
  api->PollLayers();
  if (HasCredentials())
    request_timer.Schedule(std::chrono::seconds{1});
}

void
SkySightClient::MaybeCleanupFiles() noexcept
{
  if (!forecast_cleanup_pending)
    return;

  const auto now = std::chrono::steady_clock::now();
  if (now < next_cleanup_check)
    return;

  next_cleanup_check = now + CLEANUP_CHECK_INTERVAL;
  forecast_cleanup_pending = !CleanupFiles();
}

bool
SkySightClient::CleanupFiles() noexcept
{
  return SkySightCache::Cleanup(GetCachePath());
}

std::size_t
SkySightClient::NumLayers() const noexcept
{
  return api->NumLayers();
}

const SkySight::Layer *
SkySightClient::GetLayer(std::size_t index) const noexcept
{
  return api->GetLayer(index);
}

const std::vector<SkySightRegionEntry> &
SkySightClient::GetRegions() const noexcept
{
  return api->GetRegions();
}

std::string_view
SkySightClient::GetRegion() const noexcept
{
  return api->GetRegion();
}

std::string_view
SkySightClient::GetRegionTimeZone() const noexcept
{
  const auto region_id = GetRegion();
  for (const auto &candidate : GetRegions())
    if (candidate.id == region_id)
      return candidate.tz;

  return {};
}

RoughTimeDelta
SkySightClient::GetForecastDisplayOffset(time_t utc_time) const noexcept
{
  return SkySight::GetRegionUtcOffset(GetRegionTimeZone(), utc_time);
}

std::size_t
SkySightClient::NumSelectedLayers() const noexcept
{
  return api->NumSelectedLayers();
}

const SkySight::Layer *
SkySightClient::GetSelectedLayer(std::size_t index) const noexcept
{
  return api->GetSelectedLayer(index);
}

const SkySight::Layer *
SkySightClient::GetSelectedLayer(std::string_view id) const noexcept
{
  return api->GetSelectedLayer(id);
}

bool
SkySightClient::IsSelectedLayer(std::string_view id) const noexcept
{
  return api->IsSelectedLayer(id);
}

bool
SkySightClient::SelectedLayersFull() const noexcept
{
  return api->SelectedLayersFull();
}

bool
SkySightClient::HasCredentials() const noexcept
{
  return api->HasCredentials();
}

bool
SkySightClient::IsAutoUpdateEnabled() const noexcept
{
  return CommonInterface::GetComputerSettings().weather.skysight.auto_update;
}

void
SkySightClient::OnAutoUpdateChanged() noexcept
{
  manual_update_requested = false;
  planned_live_tiles.clear();
  forecast_image_dirty = true;

  if (!IsAutoUpdateEnabled())
    api->CancelTileDownloads();
  else if (active_layer != nullptr) {
    try {
      (void)SetLayerActive(active_layer->id);
    } catch (...) {
      LogError(std::current_exception(),
               "SkySight automatic update enabling failed");
    }
  }

  OnDataUpdated();
}

bool
SkySightClient::IsThrottled() const noexcept
{
  return api->IsThrottled();
}

bool
SkySightClient::IsLiveViewUpdating(std::string_view layer_id) const noexcept
{
  return active_layer != nullptr && active_layer->id == layer_id &&
    api->HasPendingTileDownloads();
}

bool
SkySightClient::HasDownloadActivity() const noexcept
{
  for (std::size_t i = 0; i < NumSelectedLayers(); ++i) {
    const auto *layer = GetSelectedLayer(i);
    if (layer != nullptr &&
        (layer->HasPendingForecastMetadata() || layer->decoding ||
         layer->pending_downloads > 0))
      return true;
  }

  return api->HasPendingTileDownloads();
}

time_t
SkySightClient::GetThrottleRemainingSeconds() const noexcept
{
  return api->GetThrottleRemainingSeconds();
}

time_t
SkySightClient::GetDatafilesRetryRemainingSeconds() const noexcept
{
  return api->GetDatafilesRetryRemainingSeconds();
}

std::string_view
SkySightClient::GetActiveLayerId() const noexcept
{
  return active_layer != nullptr
    ? std::string_view{active_layer->id}
    : std::string_view{};
}

std::string_view
SkySightClient::GetDisplayedLayerId() const noexcept
{
  return displayed_layer != nullptr
    ? std::string_view{displayed_layer->id}
    : std::string_view{};
}

bool
SkySightClient::AddSelectedLayer(std::string_view id)
{
  return AddSelectedLayer(id, true, true);
}

bool
SkySightClient::AddSelectedLayer(std::string_view id, bool save_profile,
                           bool request_datafiles)
{
  if (id.empty() || api->SelectedLayersFull() || api->IsSelectedLayer(id))
    return false;

  auto *layer = api->GetLayer(id);
  if (layer == nullptr)
    return false;

  if (!layer->SupportsLiveTiles()) {
    const auto cached_times = SkySightCache::CollectForecastTimes(GetCachePath(),
                                                                  GetRegion(),
                                                                  layer->id);
    if (!cached_times.empty()) {
      SkySight::MergeCachedForecastTimes(*layer, cached_times,
                                         std::time(nullptr));

      const auto candidate = SkySightCache::FindForecastImage(GetCachePath(),
                                                              GetRegion(),
                                                              layer->id,
                                                              layer->forecast_time);
      if (candidate.path != nullptr &&
          candidate.forecast_time == layer->forecast_time) {
        layer->mtime = std::chrono::system_clock::to_time_t(
          File::GetLastModification(candidate.path));
      }
    }
  }

  if (!layer->SupportsLiveTiles() && request_datafiles)
    layer->RequestForecastMetadata(
      SkySight::ForecastMetadataIntent::Refresh);

  if (!api->AddSelectedLayer(id))
    return false;

  if (save_profile)
    SaveSelectedLayers();

  if (request_datafiles)
    api->PollSelectedDatafiles();
  return true;
}

bool
SkySightClient::RemoveSelectedLayer(std::string_view id)
{
  if (!api->RemoveSelectedLayer(id))
    return false;

  SaveSelectedLayers();
  return true;
}

bool
SkySightClient::HasForecastLayers() const noexcept
{
  for (std::size_t i = 0; i < api->NumLayers(); ++i) {
    const auto *layer = api->GetLayer(i);
    if (layer != nullptr && !layer->SupportsLiveTiles())
      return true;
  }

  return false;
}

bool
SkySightClient::IsForecastDecodeAvailable() const noexcept
{
  return SkySightFileDecoder::IsNetCdfDecodeAvailable();
}

void
SkySightClient::RefreshCatalog() noexcept
{
  MaybeCleanupFiles();
  api->PollRegions();
  api->PollLayers();
}

SkySightCache::Usage
SkySightClient::GetCacheUsage() const noexcept
{
  return SkySightCache::GetUsage(GetCachePath());
}

SkySightCache::Usage
SkySightClient::ClearDownloadedData() noexcept
{
  ResetTiles();
  const auto deleted = api->ClearDownloadedData();
  forecast_image_dirty = true;
  if (IsAutoUpdateEnabled() && active_layer != nullptr) {
    try {
      (void)SetLayerActive(active_layer->id);
    } catch (...) {
      LogError(std::current_exception(),
               "SkySight cache clear refresh failed");
    }
  }
  OnDataUpdated();
  return deleted;
}

bool
SkySightClient::SelectForecastTime(std::string_view id, time_t forecast_time,
                             bool download)
{
  if (forecast_time <= 0)
    return false;

  auto *layer = api->GetLayer(id);
  if (layer == nullptr || !api->IsSelectedLayer(id) ||
      layer->SupportsLiveTiles())
    return false;

  const auto *datafile = layer->FindDatafile(forecast_time);
  if (datafile == nullptr)
    return false;

  layer->forecast_time_mode = SkySight::ForecastTimeMode::Fixed;
  layer->forecast_time = forecast_time;
  CommonInterface::SetUIState().weather.skysight.cursor_initialized = true;

  if (!SyncCachedForecastImage(GetRegion(), *layer, forecast_time) &&
      download) {
    if (!api->QueueForecastDatafile(id, datafile->time, datafile->link))
      return false;
  }

  if (active_layer == layer)
    tile_filenames[0].clear();

  OnDataUpdated();
  return true;
}

bool
SkySightClient::SelectAutomaticForecastTime(std::string_view id, bool download)
{
  auto *layer = api->GetLayer(id);
  if (layer == nullptr || !api->IsSelectedLayer(id) ||
      layer->SupportsLiveTiles())
    return false;

  const auto forecast_time = SkySight::ChooseAutomaticForecastTime(*layer);
  if (forecast_time <= 0)
    return false;

  const auto *datafile = layer->FindDatafile(forecast_time);
  if (datafile == nullptr)
    return false;

  layer->forecast_time_mode = SkySight::ForecastTimeMode::AutoDefault;
  layer->forecast_time = forecast_time;

  if (!SyncCachedForecastImage(GetRegion(), *layer, forecast_time) &&
      download) {
    if (!api->QueueForecastDatafile(id, datafile->time, datafile->link))
      return false;
  }

  if (active_layer == layer)
    tile_filenames[0].clear();

  OnDataUpdated();
  return true;
}

bool
SkySightClient::PreloadForecast(std::string_view id) noexcept
{
  return api->PreloadDatafiles(id);
}

bool
SkySightClient::PreloadAllForecasts() noexcept
{
  return api->PreloadAllDatafiles();
}

unsigned
SkySightClient::GetPreloadFileCount() const
{
  unsigned count = 0;
  const auto now = std::time(nullptr);
  for (std::size_t i = 0; i < api->NumSelectedLayers(); ++i) {
    const auto *layer = api->GetSelectedLayer(i);
    if (layer != nullptr && !layer->SupportsLiveTiles())
      count += SkySight::GetForecastPreloadDatafiles(*layer, now).size();
  }

  return count;
}

unsigned
SkySightClient::GetSelectedForecastLayerCount() const noexcept
{
  unsigned count = 0;
  for (std::size_t i = 0; i < api->NumSelectedLayers(); ++i) {
    const auto *layer = api->GetSelectedLayer(i);
    if (layer != nullptr && !layer->SupportsLiveTiles())
      ++count;
  }

  return count;
}

void
SkySightClient::ReloadSelectedLayersFromProfile()
{
  api->ClearSelectedLayers();

  const char *configured_layers = Profile::Get(ProfileKeys::SkySightSelectedLayers);
  if (configured_layers == nullptr)
    configured_layers = Profile::Get(ProfileKeys::LegacySkySightSelectedLayers);
  if (configured_layers == nullptr || *configured_layers == '\0')
    return;

  std::string remaining{configured_layers};
  while (!remaining.empty()) {
    const auto split = remaining.find(',');
    const auto layer_id = remaining.substr(0, split);
    if (!layer_id.empty())
      (void)AddSelectedLayer(layer_id, false, false);

    if (split == std::string::npos)
      break;

    remaining.erase(0, split + 1);
  }
}

void
SkySightClient::SaveSelectedLayers() const
{
  std::string value;

  for (std::size_t i = 0; i < api->NumSelectedLayers(); ++i) {
    const auto *layer = api->GetSelectedLayer(i);
    if (layer == nullptr)
      continue;

    if (!value.empty())
      value.push_back(',');

    value += layer->id;
  }

  Profile::Set(ProfileKeys::SkySightSelectedLayers, value.c_str());
}

void
SkySightClient::OnLayerCatalogChanged(std::string_view active_id,
                                std::string_view displayed_id) noexcept
{
  active_layer = active_id.empty()
    ? nullptr
    : api->GetLayer(active_id);
  displayed_layer = displayed_id.empty()
    ? nullptr
    : api->GetLayer(displayed_id);

  if (active_layer == nullptr) {
    ResetTiles();
  }
}

void
SkySightClient::ResetTiles() noexcept
{
#ifdef ENABLE_OPENGL
  if (auto *map = UIGlobals::GetMap())
    for (unsigned i = 0; i < tile_filenames.size(); ++i)
      map->SetOverlay(i, nullptr);
#endif

  for (std::size_t i = 0; i < tile_filenames.size(); ++i) {
    tile_filenames[i].clear();
    tile_coordinates[i] = {};
    tile_timestamps[i] = 0;
  }

  forecast_image_dirty = true;
  planned_live_timestamp_known = false;
  planned_live_timestamp = 0;
  planned_live_bounds.SetInvalid();
  planned_live_tiles.clear();
  displayed_layer = nullptr;
}

bool
SkySightClient::SetLayerActive(std::string_view id, bool request_update)
{
  auto *layer = api->GetLayer(id);
  if (layer == nullptr)
    return false;

  if (!api->IsSelectedLayer(id) && !AddSelectedLayer(id))
    return false;

  if (active_layer != layer)
    api->CancelTileDownloads();

  active_layer = layer;
  manual_update_requested = request_update;
  if (!active_layer->SupportsLiveTiles()) {
    if (api->IsSelectedLayer(id)) {
      /* Restore AUTO's catalog default when the page has not picked a
         step yet, so the cache lookup can match a just-fetched layer. */
      if (active_layer->UsesAutomaticForecastTime() &&
          active_layer->forecast_time <= 0 &&
          !active_layer->forecast_datafiles.empty())
        active_layer->forecast_time =
          SkySight::ChooseAutomaticForecastTime(*active_layer);

      const bool has_exact_forecast_image =
        HasExactForecastImage(GetRegion(), *active_layer);
      const bool manual_update_started = request_update &&
        !active_layer->forecast_datafiles.empty() &&
        SelectAutomaticForecastTime(id, true);

      if (!manual_update_started && !has_exact_forecast_image &&
          (IsAutoUpdateEnabled() || request_update))
        (void)api->PreloadDefaultDatafile(id);
      else if (!manual_update_started && !has_exact_forecast_image)
        api->RequestForecastMetadata(id);
    }
  }
  ResetTiles();
  if (!layer->SupportsLiveTiles())
    manual_update_requested = false;
  OnDataUpdated();
  return true;
}

void
SkySightClient::ApplyPageOverlay(const PageLayout &page) noexcept
{
  try {
    if (!page.UsesSkySightOverlay()) {
      if (!GetActiveLayerId().empty())
        DeactivateLayer();

      return;
    }

    const auto overlay_id = std::string_view{page.skysight_overlay.c_str()};
    auto *layer = api->GetLayer(overlay_id);
    if (layer == nullptr)
      return;

    const bool automatic = layer->SupportsLiveTiles() ||
      page.skysight_time == PageLayout::SKYSIGHT_TIME_AUTO;
    layer->forecast_time_mode = automatic
      ? SkySight::ForecastTimeMode::AutoDefault
      : SkySight::ForecastTimeMode::Fixed;
    if (automatic)
      /* Keep a usable catalog time so SetLayerActive can reuse a
         cached image instead of treating AUTO as a cache miss. */
      layer->forecast_time =
        SkySight::ChooseAutomaticForecastTime(*layer);
    else {
      const time_t fixed = time_t(page.skysight_time);
      if (int64_t(fixed) == page.skysight_time)
        layer->forecast_time = fixed;
      else {
        layer->forecast_time_mode = SkySight::ForecastTimeMode::AutoDefault;
        layer->forecast_time = 0;
      }
    }

    if (GetActiveLayerId() != overlay_id)
      (void)SetLayerActive(overlay_id);

    if (!layer->SupportsLiveTiles() && !layer->forecast_datafiles.empty()) {
      if (layer->UsesAutomaticForecastTime())
        (void)SelectAutomaticForecastTime(overlay_id,
                                          IsAutoUpdateEnabled());
      else if (layer->FindDatafile(layer->forecast_time) != nullptr)
        (void)SelectForecastTime(overlay_id, layer->forecast_time,
                                 IsAutoUpdateEnabled());
    }
  } catch (...) {
    LogError(std::current_exception(), "SkySight page overlay selection failed");
  }
}

void
SkySightClient::DeactivateLayer()
{
  api->CancelTileDownloads();
  active_layer = nullptr;
  manual_update_requested = false;
  ResetTiles();
  OnDataUpdated();
}

void
SkySightClient::OnDataUpdated() noexcept
{
  forecast_image_dirty = true;
  planned_live_tiles.clear();

  if (auto *map = UIGlobals::GetMapIfActive())
    map->DeferRedraw();

  if (CommonInterface::main_window != nullptr)
    CommonInterface::main_window->SendCalculatedUpdate();
}

void
SkySightClient::OnForecastThrottled() noexcept
{
  if (throttle_notification_active)
    return;

  throttle_notification_active = true;
  StaticString<128> message;
  message.Format(_("SkySight rate limit reached; continuing in %u seconds."),
                 unsigned(GetThrottleRemainingSeconds()));
  Message::AddMessage(message.c_str());
}

void
SkySightClient::OnForecastResumed() noexcept
{
  if (!throttle_notification_active)
    return;

  throttle_notification_active = false;
  Message::AddMessage(_("SkySight downloads resumed."));
}

void
SkySightClient::OnForecastProgressCancelled() noexcept
{
  if (!forecast_progress_visible)
    return;

  BackgroundDownloadProgress::Get().End();
  forecast_progress_visible = false;
}

void
SkySightClient::OnForecastProgress(const SkySight::ForecastProgress &progress) noexcept
{
  auto &download_progress = BackgroundDownloadProgress::Get();
  StaticString<128> text;
  const unsigned available = progress.completed > progress.failed
    ? progress.completed - progress.failed
    : 0;

  switch (progress.phase) {
  case SkySight::ForecastProgressPhase::Metadata:
    text = _("Loading SkySight forecast steps...");
    break;

  case SkySight::ForecastProgressPhase::Download:
  case SkySight::ForecastProgressPhase::Decode:
    text.Format(_("SkySight forecasts: %u of %u available..."),
                available, progress.total);
    break;

  case SkySight::ForecastProgressPhase::Throttled:
    text.Format(_("SkySight rate limited; continuing in %u seconds..."),
                progress.retry_seconds);
    break;

  case SkySight::ForecastProgressPhase::RetryWait:
    text.Format(_("SkySight connection failed; retrying in %u seconds..."),
                progress.retry_seconds);
    break;

  case SkySight::ForecastProgressPhase::Complete:
    if (progress.failed > 0)
      text.Format(_("SkySight preload completed with %u failures."),
                  progress.failed);
    break;
  }

  if (!forecast_progress_visible &&
      progress.phase != SkySight::ForecastProgressPhase::Complete) {
    download_progress.Begin(text.c_str());
    forecast_progress_visible = true;
  } else if (forecast_progress_visible && !text.empty()) {
    download_progress.SetText(text.c_str());
  }

  if (forecast_progress_visible) {
    download_progress.SetProgressRange(std::max(1u, progress.total));
    download_progress.SetProgressPosition(std::min(progress.completed,
                                                    std::max(1u, progress.total)));
  }

  if (progress.phase == SkySight::ForecastProgressPhase::Complete &&
      forecast_progress_visible) {
    download_progress.End();
    forecast_progress_visible = false;

    StaticString<160> summary;
    if (progress.failed == 0)
      summary.Format(_("SkySight offline cache ready: %u files."),
                     progress.completed);
    else {
      const unsigned ready = progress.completed > progress.failed
        ? progress.completed - progress.failed
        : 0;
      summary.Format(_("SkySight cache finished: %u of %u files ready, %u failed."),
                     ready, progress.total, progress.failed);
    }
    Message::AddMessage(summary.c_str());
  }
}

bool
SkySightClient::UpdateActiveLayer(unsigned index, Path path,
                            const GeoBitmap::TileData &tile)
{
#ifndef ENABLE_OPENGL
  (void)index;
  (void)path;
  (void)tile;
  return false;
#else
  if (active_layer == nullptr)
    return false;

  auto *map = UIGlobals::GetMap();
  if (map == nullptr)
    return false;

  std::unique_ptr<MapOverlayBitmap> bitmap;
  try {
    bitmap = std::make_unique<MapOverlayBitmap>(path);
  } catch (...) {
    return false;
  }

  bitmap->SetAlpha(active_layer->alpha);

  StaticString<160> label;
  label.Format("SkySight: %s", active_layer->name.c_str());
  if (active_layer->SupportsLiveTiles()) {
    label.AppendFormat(" (%u/%u/%u)", tile.zoom, tile.x, tile.y);
  } else if (active_layer->forecast_time != 0) {
    const auto forecast_time = FormatLocalDateTimeYYYYMMDDHHMM(
      TimeStamp(std::chrono::duration<double>(active_layer->forecast_time)),
      GetForecastDisplayOffset(active_layer->forecast_time));
    label.AppendFormat(" (%s)", forecast_time.c_str());
  }

  bitmap->SetLabel(label.c_str());

  map->SetOverlay(index, std::move(bitmap));
  return true;
#endif
}

bool
SkySightClient::DisplayForecastLayer()
{
#ifndef ENABLE_OPENGL
  return false;
#else
  auto *map_window = UIGlobals::GetMapIfActive();
  if (map_window == nullptr || active_layer == nullptr)
    return false;

  if (displayed_layer != active_layer) {
    ResetTiles();
    displayed_layer = active_layer;
  }

  if (active_layer->ShouldShowUpdating())
    api->PollSelectedDatafiles();

  if (!forecast_image_dirty) {
    if (!tile_filenames[0].empty() && File::Exists(Path{tile_filenames[0].c_str()}))
      return true;

    forecast_image_dirty = true;
  }

  const auto candidate = SkySightCache::FindForecastImage(GetCachePath(),
                                                          GetRegion(),
                                                          active_layer->id,
                                                          active_layer->forecast_time);
  if (candidate.path == nullptr) {
    map_window->SetOverlay(0, nullptr);
    tile_filenames[0].clear();
    forecast_image_dirty = false;
    return false;
  }

  if (candidate.forecast_time == active_layer->forecast_time) {
    active_layer->mtime = std::chrono::system_clock::to_time_t(
      File::GetLastModification(candidate.path));
  }

  if (tile_filenames[0] != candidate.path.c_str()) {
    if (!UpdateActiveLayer(0, candidate.path,
                           GeoBitmap::TileData{0, 0, 0})) {
      map_window->SetOverlay(0, nullptr);
      tile_filenames[0].clear();
      forecast_image_dirty = false;
      return false;
    }

    tile_filenames[0] = candidate.path.c_str();
  }

  for (unsigned i = 1; i < tile_filenames.size(); ++i) {
    if (!tile_filenames[i].empty()) {
      map_window->SetOverlay(i, nullptr);
      tile_filenames[i].clear();
    }
  }

  forecast_image_dirty = false;
  return true;
#endif
}

bool
SkySightClient::DisplayTileLayer()
{
#ifndef ENABLE_OPENGL
  return false;
#else
  auto *map_window = UIGlobals::GetMapIfActive();
  if (map_window == nullptr || active_layer == nullptr)
    return false;

  const auto map_tile = GeoBitmap::GetTile(map_window->VisibleProjection(),
                                           active_layer->zoom_min,
                                           SkySight::GetLiveTileMapZoomMaximum(
                                             active_layer->zoom_max));
  const auto map_bounds = map_window->VisibleProjection().GetScreenBounds();
  if (!map_bounds.Check() || !map_bounds.IsValid())
    return false;

  const auto live_zoom = SkySight::SelectLiveTileZoom(map_tile.zoom,
                                                       active_layer->zoom_min);
  const auto base_tile = GeoBitmap::GetTile(map_bounds, live_zoom);
  GeoBounds region_bounds = GeoBounds::Invalid();
  for (const auto &candidate : api->GetRegions())
    if (candidate.id == api->GetRegion()) {
      region_bounds = candidate.bounds;
      break;
    }

  if (displayed_layer != active_layer) {
    ResetTiles();
    displayed_layer = active_layer;
  }

  const time_t current_slot = (std::time(nullptr) / 600) * 600;
  const bool probe_next_slot = !active_layer->HasKnownLiveTimestamp() ||
    (active_layer->live_timestamp_from_probe &&
     active_layer->last_update < current_slot);
  const bool has_known_timestamp = !probe_next_slot;
  const time_t refresh_time = probe_next_slot
    ? current_slot
    : active_layer->last_update;
  const auto visible_tiles = CollectVisibleLiveTiles(
    map_bounds, base_tile, region_bounds, LIVE_TILE_RANGE_OFFSET);

  if (!planned_live_tiles.empty() &&
      planned_live_timestamp_known == has_known_timestamp &&
      planned_live_timestamp == refresh_time &&
      IsSameBounds(planned_live_bounds, map_bounds) &&
      IsSameTileSequence(planned_live_tiles, visible_tiles)) {
    return !visible_tiles.empty();
  }

  planned_live_timestamp_known = has_known_timestamp;
  planned_live_timestamp = refresh_time;
  planned_live_bounds = map_bounds;
  planned_live_tiles = visible_tiles;

  LiveTileCacheIndex cache{*api, *active_layer};
  std::vector<TargetLiveTile> target_tiles;
  target_tiles.reserve(visible_tiles.size());
  for (const auto &visible : visible_tiles) {
    target_tiles.push_back({
      visible,
      cache.Find(visible, refresh_time).exists,
    });
  }

  std::array<unsigned, SkySight::RECENT_LIVE_TILE_FALLBACK_STEPS>
    timestamp_coverage{};
  for (unsigned step = 0; step < timestamp_coverage.size(); ++step) {
    const auto candidate_time = refresh_time -
      time_t(step) * SkySight::LIVE_TILE_INTERVAL_SECONDS;

    for (const auto &target : target_tiles) {
      bool covered = cache.Find(target.tile, candidate_time).exists;
      for (unsigned zoom = target.tile.zoom;
           !covered && zoom > active_layer->zoom_min;) {
        --zoom;
        covered = cache.Find(SkySight::GetTileAncestor(target.tile, zoom),
                             candidate_time).exists;
      }

      if (covered)
        ++timestamp_coverage[step];
    }

    for (std::size_t i = 0; i < tile_filenames.size(); ++i)
      if (!tile_filenames[i].empty() &&
          tile_coordinates[i].zoom > live_zoom &&
          tile_timestamps[i] == candidate_time &&
          GeoBitmap::GetBounds(tile_coordinates[i]).Overlaps(map_bounds))
        ++timestamp_coverage[step];
  }

  const time_t display_timestamp =
    SkySight::SelectCoherentLiveTileTimestamp(refresh_time,
                                               timestamp_coverage);

  std::vector<DisplayLiveTile> fallback_tiles;
  std::vector<DisplayLiveTile> target_zoom_tiles;
  fallback_tiles.reserve(target_tiles.size());
  target_zoom_tiles.reserve(target_tiles.size());

  for (const auto &target : target_tiles) {
    const auto &cached_target = cache.Find(target.tile, display_timestamp);
    if (cached_target.exists) {
      AppendUniqueLiveTile(target_zoom_tiles,
                           {target.tile, cached_target.timestamp,
                            cached_target.path.c_str()});
      continue;
    }

    for (unsigned zoom = target.tile.zoom; zoom > active_layer->zoom_min;) {
      --zoom;
      const auto ancestor = SkySight::GetTileAncestor(target.tile, zoom);
      const auto &cached = cache.Find(ancestor, display_timestamp);
      if (!cached.exists)
        continue;

      AppendUniqueLiveTile(fallback_tiles,
                           {ancestor, cached.timestamp, cached.path.c_str()});
      break;
    }
  }

  std::vector<DisplayLiveTile> display_tiles;
  display_tiles.reserve(fallback_tiles.size() + target_zoom_tiles.size());
  display_tiles.insert(display_tiles.end(), fallback_tiles.begin(),
                       fallback_tiles.end());
  display_tiles.insert(display_tiles.end(), target_zoom_tiles.begin(),
                       target_zoom_tiles.end());

  const bool target_view_complete = !target_tiles.empty() &&
    std::all_of(target_tiles.begin(), target_tiles.end(),
                [](const auto &target) {
                  return target.exact_refresh_available;
                });
  if (!target_view_complete) {
    /* When zooming out, already displayed children cannot be discovered by
       walking the new target's ancestors.  Keep visible children until the
       coarser target view has arrived. */
    for (std::size_t i = 0; i < tile_filenames.size(); ++i) {
      const auto &tile = tile_coordinates[i];
      if (tile_filenames[i].empty() || tile.zoom <= live_zoom ||
          tile_timestamps[i] != display_timestamp ||
          !GeoBitmap::GetBounds(tile).Overlaps(map_bounds))
        continue;

      AppendUniqueLiveTile(display_tiles,
                           {tile, tile_timestamps[i], tile_filenames[i]});
    }
  }

  unsigned slot = 0;
  for (const auto &display : display_tiles) {
    if (slot >= tile_filenames.size())
      break;

    if (tile_filenames[slot] != display.path) {
      if (UpdateActiveLayer(slot, Path{display.path.c_str()}, display.tile)) {
        tile_filenames[slot] = display.path;
        tile_coordinates[slot] = display.tile;
        tile_timestamps[slot] = display.timestamp;
      }
    } else {
      tile_coordinates[slot] = display.tile;
      tile_timestamps[slot] = display.timestamp;
    }
    ++slot;
  }

  while (slot < tile_filenames.size()) {
    if (!tile_filenames[slot].empty()) {
      map_window->SetOverlay(slot, nullptr);
      tile_filenames[slot].clear();
    }
    tile_coordinates[slot] = {};
    tile_timestamps[slot] = 0;
    ++slot;
  }

  std::set<std::string, std::less<>> desired_keys;
  std::vector<GeoBitmap::TileData> missing_target_tiles;
  missing_target_tiles.reserve(target_tiles.size());
  const bool download_allowed = IsAutoUpdateEnabled() ||
    manual_update_requested;

  if (download_allowed && has_known_timestamp) {
    for (const auto &target : target_tiles) {
      const auto &exact = cache.Find(target.tile, refresh_time);
      desired_keys.emplace(exact.path.c_str());
      if (!target.exact_refresh_available)
        missing_target_tiles.push_back(target.tile);
    }
  }

  if (download_allowed && !has_known_timestamp && !target_tiles.empty()) {
    /* /data/last_updated does not currently publish timestamps for every
       live pseudo-layer.  Probe one centre tile only; once it succeeds, normal
       viewport downloads begin centre-out. */
    const auto probe_tile = target_tiles.front().tile;
    const auto &probe = cache.Find(probe_tile, refresh_time);
    desired_keys.emplace(probe.path.c_str());
    if (probe.exists) {
      active_layer->last_update = refresh_time;
      active_layer->live_timestamp_from_probe = true;
      api->OnLiveTileProbeSucceeded(active_layer->id, refresh_time);
    } else {
      missing_target_tiles.push_back(probe_tile);
    }
  }

  api->ReconcileTileDownloads(desired_keys);
  for (const auto &tile : missing_target_tiles)
    api->EnsureTile(*active_layer, refresh_time, tile);

  if (manual_update_requested && target_view_complete)
    manual_update_requested = false;

  return !visible_tiles.empty();
#endif
}

void
SkySightClient::Render()
{
#ifndef ENABLE_OPENGL
  return;
#else
  assert(InMainThread());

  if (active_layer == nullptr)
    return;

  if (active_layer->SupportsLiveTiles())
    (void)DisplayTileLayer();
  else
    (void)DisplayForecastLayer();
#endif
}
