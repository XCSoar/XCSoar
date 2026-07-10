// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightClient.hpp"
#include "SkySightCache.hpp"
#include "SkySightAPI.hpp"
#include "ForecastUtils.hpp"
#include "SkySightFileDecoder.hpp"
#include "Weather/BackgroundDownloadProgress.hpp"
#include "Weather/MapOverlay/PagePlacement.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
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

namespace {

[[nodiscard]] static bool
HasExactForecastImage(std::string_view region,
                      const SkySight::Layer &layer) noexcept
{
  if (layer.forecast_time <= 0)
    return false;

  const auto candidate = SkySightCache::FindForecastImage(SkySightClient::GetLocalPath(),
                                                          region,
                                                          layer.id,
                                                          layer.forecast_time);
  return candidate.path != nullptr &&
    candidate.forecast_time == layer.forecast_time &&
    File::Exists(candidate.path);
}

[[nodiscard]] bool
SyncCachedForecastImage(std::string_view region,
                        SkySight::Layer &layer,
                        SkySight::Layer &selected,
                        time_t forecast_time) noexcept
{
  const auto candidate = SkySightCache::FindForecastImage(
    SkySightClient::GetLocalPath(), region, layer.id, forecast_time);
  if (candidate.path == nullptr || candidate.forecast_time != forecast_time)
    return false;

  const auto mtime = std::chrono::system_clock::to_time_t(
    File::GetLastModification(candidate.path));
  layer.mtime = mtime;
  selected.mtime = mtime;
  return true;
}

} // namespace
SkySightClient::SkySightClient(CurlGlobal &curl)
  :api(std::make_unique<SkySightAPI>(*this, curl, GetLocalPath())),
   request_timer([this]{ PollPendingDatafiles(); })
{
  Init();
}

SkySightClient::~SkySightClient() = default;

AllocatedPath
SkySightClient::GetLocalPath() noexcept
{
  return MakeCacheDirectory("skysight");
}

void
SkySightClient::Init()
{
  CleanupFiles();
  forecast_cleanup_pending = !SkySightCache::IsTrustedTimeAvailableForCleanup();

  ResetTiles();
  active_layer = nullptr;

  const auto &settings = CommonInterface::GetComputerSettings().weather.skysight;
  api->Configure(settings.email.c_str(), settings.password.c_str(),
                 settings.region.c_str());
  ReloadSelectedLayersFromProfile();
  api->PollRegions();
  api->PollLayers();
  request_timer.Schedule(std::chrono::seconds{1});
}

void
SkySightClient::MaybeCleanupFiles() noexcept
{
  if (!forecast_cleanup_pending ||
      !SkySightCache::IsTrustedTimeAvailableForCleanup())
    return;

  CleanupFiles();
  forecast_cleanup_pending = false;
}

void
SkySightClient::CleanupFiles() noexcept
{
  SkySightCache::Cleanup(GetLocalPath());
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
  return const_cast<SkySightAPI &>(*api).GetSelectedLayer(id);
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
SkySightClient::IsThrottled() const noexcept
{
  return api->IsThrottled();
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

  auto selected = *layer;
  if (!selected.SupportsLiveTiles()) {
    selected.datafiles_pending = request_datafiles || layer->datafiles_pending;
    selected.updating = selected.ShouldShowUpdating();
  }

  if (!api->AddSelectedLayer(selected))
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

void
SkySightClient::PollPendingDatafiles() noexcept
{
  MaybeCleanupFiles();
  api->Poll();
}

bool
SkySightClient::SelectForecastTime(std::string_view id, time_t forecast_time)
{
  if (forecast_time <= 0)
    return false;

  auto *layer = api->GetLayer(id);
  auto *selected = api->GetSelectedLayer(id);
  if (layer == nullptr || selected == nullptr || layer->SupportsLiveTiles())
    return false;

  const auto *datafile = layer->FindDatafile(forecast_time);
  if (datafile == nullptr)
    return false;

  layer->forecast_time_mode = SkySight::ForecastTimeMode::Fixed;
  selected->forecast_time_mode = SkySight::ForecastTimeMode::Fixed;
  layer->forecast_time = forecast_time;
  selected->forecast_time = forecast_time;
  CommonInterface::SetUIState().weather.skysight.cursor_initialized = true;

  if (!SyncCachedForecastImage(GetRegion(), *layer, *selected,
                               forecast_time)) {
    if (!api->QueueForecastDatafile(id, datafile->time, datafile->link))
      return false;
  }

  if (active_layer == layer)
    tile_filenames[0].clear();

  OnDataUpdated();
  return true;
}

bool
SkySightClient::SelectAutomaticForecastTime(std::string_view id)
{
  auto *layer = api->GetLayer(id);
  auto *selected = api->GetSelectedLayer(id);
  if (layer == nullptr || selected == nullptr || layer->SupportsLiveTiles())
    return false;

  const auto forecast_time = SkySight::ChooseClosestForecastTime(
    layer->forecast_datafiles,
    [](const auto &candidate) noexcept {
      return candidate.time;
    });
  if (forecast_time <= 0)
    return false;

  const auto *datafile = layer->FindDatafile(forecast_time);
  if (datafile == nullptr)
    return false;

  layer->forecast_time_mode = SkySight::ForecastTimeMode::AutoDefault;
  selected->forecast_time_mode = SkySight::ForecastTimeMode::AutoDefault;
  layer->forecast_time = forecast_time;
  selected->forecast_time = forecast_time;

  if (!SyncCachedForecastImage(GetRegion(), *layer, *selected,
                               forecast_time)) {
    if (!api->QueueForecastDatafile(id, datafile->time, datafile->link))
      return false;
  }

  if (active_layer == layer)
    tile_filenames[0].clear();

  OnDataUpdated();
  return true;
}

bool
SkySightClient::SelectPageLayer(std::string_view id)
{
  if (id.empty() || !api->IsSelectedLayer(id))
    return false;

  auto &settings = CommonInterface::SetUISettings().pages;
  const auto &pages = CommonInterface::GetUIState().pages;
  if (pages.current_index >= settings.n_pages)
    return false;

  if (!WeatherMapOverlay::SetSkySightLayerOnPage(settings,
                                                 pages.current_index, id))
    return false;

  const auto &page = settings.pages[pages.current_index];
  Profile::Save(Profile::map, page, pages.current_index);
  CommonInterface::SetUIState().weather.skysight.cursor_initialized = true;

  return SetLayerActive(id);
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
SkySightClient::GetPreloadFileCount() const noexcept
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

  if (active_layer == nullptr)
    ResetTiles();
}

void
SkySightClient::ResetTiles() noexcept
{
#ifdef ENABLE_OPENGL
  if (auto *map = UIGlobals::GetMap())
    for (unsigned i = 0; i < tile_filenames.size(); ++i)
      map->SetOverlay(i, nullptr);
#endif

  for (auto &i : tile_filenames)
    i.clear();

  forecast_image_dirty = true;
  displayed_layer = nullptr;
  displayed_zoom = 0;
}

bool
SkySightClient::SetLayerActive(std::string_view id)
{
  auto *layer = api->GetLayer(id);
  if (layer == nullptr)
    return false;

  if (!api->IsSelectedLayer(id) && !AddSelectedLayer(id))
    return false;

  if (active_layer != layer)
    api->CancelTileDownloads();

  active_layer = layer;
  if (!active_layer->SupportsLiveTiles()) {
    if (auto *selected = api->GetSelectedLayer(id); selected != nullptr) {
      const bool has_exact_forecast_image =
        HasExactForecastImage(GetRegion(), *selected);

      selected->updating = selected->ShouldShowUpdating();
      active_layer->updating = selected->updating;

      if (!has_exact_forecast_image)
        (void)api->PreloadDefaultDatafile(id);
      else
        api->PollSelectedDatafiles();
    }
  }
  ResetTiles();
  OnDataUpdated();
  return true;
}

void
SkySightClient::ApplyPageOverlay(std::string_view overlay_id,
                           bool reset_automatic_time) noexcept
{
  try {
    if (overlay_id.empty()) {
      if (!GetActiveLayerId().empty())
        DeactivateLayer();

      return;
    }

    if (reset_automatic_time) {
      if (auto *layer = api->GetLayer(overlay_id); layer != nullptr &&
          !layer->SupportsLiveTiles()) {
        layer->forecast_time_mode = SkySight::ForecastTimeMode::AutoDefault;
        if (auto *selected = api->GetSelectedLayer(overlay_id);
            selected != nullptr)
          selected->forecast_time_mode = SkySight::ForecastTimeMode::AutoDefault;

        if (!layer->forecast_datafiles.empty())
          (void)SelectAutomaticForecastTime(overlay_id);
      }
    }

    if (GetActiveLayerId() != overlay_id)
      (void)SetLayerActive(overlay_id);
  } catch (...) {
    LogError(std::current_exception(), "SkySight page overlay selection failed");
  }
}

void
SkySightClient::DeactivateLayer()
{
  api->CancelTileDownloads();
  active_layer = nullptr;
  ResetTiles();
  OnDataUpdated();
}

void
SkySightClient::OnDataUpdated() noexcept
{
  MaybeCleanupFiles();
  forecast_image_dirty = true;

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
      CommonInterface::GetComputerSettings().utc_offset);
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

  if (active_layer->updating)
    api->PollSelectedDatafiles();

  if (!forecast_image_dirty) {
    if (!tile_filenames[0].empty() && File::Exists(Path{tile_filenames[0].c_str()}))
      return true;

    forecast_image_dirty = true;
  }

  const auto candidate = SkySightCache::FindForecastImage(GetLocalPath(),
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
    if (auto *selected = api->GetSelectedLayer(active_layer->id);
        selected != nullptr)
      selected->mtime = active_layer->mtime;
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

  api->PollLastUpdates();

  const auto base_tile = GeoBitmap::GetTile(map_window->VisibleProjection(),
                                            active_layer->zoom_min,
                                            active_layer->zoom_max);
  const auto map_bounds = map_window->VisibleProjection().GetScreenBounds();
  if (!map_bounds.Check() || !map_bounds.IsValid())
    return false;

  if (displayed_layer != active_layer || displayed_zoom != base_tile.zoom) {
    ResetTiles();
    displayed_layer = active_layer;
    displayed_zoom = base_tile.zoom;
  }

  const time_t current_slot = (std::time(nullptr) / 600) * 600;
  const bool probe_next_slot = !active_layer->HasKnownLiveTimestamp() ||
    (active_layer->live_timestamp_from_probe &&
     active_layer->last_update < current_slot);
  const bool has_known_timestamp = !probe_next_slot;
  const time_t refresh_time = probe_next_slot
    ? current_slot
    : active_layer->last_update;
  const int tiles_per_axis = 1 << base_tile.zoom;
  const auto normalize_x = [tiles_per_axis](int value) {
    int result = value % tiles_per_axis;
    if (result < 0)
      result += tiles_per_axis;

    return (uint16_t)result;
  };
  bool any_visible = false;
  bool probe_queued = false;
  unsigned slot = 0;
  constexpr int tile_range = int(LIVE_TILE_RANGE_OFFSET);
  for (int x = int(base_tile.x) - tile_range;
       x <= int(base_tile.x) + tile_range; ++x) {
    for (int y = int(base_tile.y) - tile_range;
         y <= int(base_tile.y) + tile_range; ++y, ++slot) {
      if (y < 0 || y >= tiles_per_axis) {
        map_window->SetOverlay(slot, nullptr);
        tile_filenames[slot].clear();
        continue;
      }

      GeoBitmap::TileData tile{base_tile.zoom, normalize_x(x), (uint16_t)y};

      if (!GeoBitmap::GetBounds(tile).Overlaps(map_bounds)) {
        map_window->SetOverlay(slot, nullptr);
        tile_filenames[slot].clear();
        continue;
      }

      any_visible = true;
      bool found = false;
      const unsigned fallback_steps = has_known_timestamp ? 3 : 24;
      for (unsigned step = 0; step < fallback_steps; ++step) {
        const auto candidate_time = refresh_time - (time_t(step) * 600);
        const auto path = api->GetTilePath(*active_layer, candidate_time, tile);
        if (!File::Exists(path))
          continue;

        if (tile_filenames[slot] != path.c_str()) {
          if (UpdateActiveLayer(slot, path, tile))
            tile_filenames[slot] = path.c_str();
        }

        found = true;
        break;
      }

      if (has_known_timestamp && !found) {
        api->EnsureTile(*active_layer, refresh_time, tile);
        map_window->SetOverlay(slot, nullptr);
        tile_filenames[slot].clear();
      } else if (!has_known_timestamp && !probe_queued) {
        /* /data/last_updated does not currently publish timestamps for every
           live pseudo-layer.  Probe one visible tile only; once it succeeds,
           the cached file below establishes the timestamp without fanning an
           unverified timestamp out across the whole viewport. */
        const auto probe_path =
          api->GetTilePath(*active_layer, refresh_time, tile);
        if (File::Exists(probe_path)) {
          active_layer->last_update = refresh_time;
          active_layer->live_timestamp_from_probe = true;
          api->OnLiveTileProbeSucceeded(active_layer->id, refresh_time);
          probe_queued = true;
        } else {
          api->EnsureTile(*active_layer, refresh_time, tile);
          probe_queued = true;
        }
      }
    }
  }

  return any_visible;
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
