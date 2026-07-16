// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightManager.hpp"
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
#include "thread/Debug.hpp"

#include <cassert>
#include "MapWindow/OverlayBitmap.hpp"
#include "system/FileUtil.hpp"
#include <algorithm>
#include <chrono>
#include <exception>

namespace {

void
MigrateCacheFiles(Path source_path, Path destination_path) noexcept
{
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
  }
}

[[nodiscard]] static bool
HasExactForecastImage(std::string_view region,
                      const SkySight::Layer &layer) noexcept
{
  if (layer.forecast_time <= 0)
    return false;

  const auto candidate = SkySightCache::FindForecastImage(
    SkySightManager::GetCachePath(), region, layer.id, layer.forecast_time);
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
    SkySightManager::GetCachePath(), region, layer.id, forecast_time);
  if (candidate.path == nullptr || candidate.forecast_time != forecast_time)
    return false;

  const auto mtime = std::chrono::system_clock::to_time_t(
    File::GetLastModification(candidate.path));
  layer.mtime = mtime;
  selected.mtime = mtime;
  return true;
}

} // namespace
SkySightManager::SkySightManager(CurlGlobal &curl)
  :api(std::make_unique<SkySightAPI>(*this, curl, GetCachePath())),
   request_timer([this]{ PollPendingDatafiles(); })
{
  Init();
}

SkySightManager::~SkySightManager() = default;

AllocatedPath
SkySightManager::GetCachePath() noexcept
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
SkySightManager::Init()
{
  throttle_notification_active = false;

  const auto cache_path = GetCachePath();
  MigrateCacheFiles(MakeCacheDirectory("skysight"), cache_path);
  MigrateCacheFiles(AllocatedPath::Build(LocalPath("weather"), "skysight"),
                    cache_path);
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

  if (api->HasCredentials())
    request_timer.Schedule(std::chrono::seconds{1});
  else
    request_timer.Cancel();
}

void
SkySightManager::MaybeCleanupFiles() noexcept
{
  if (!forecast_cleanup_pending ||
      !SkySightCache::IsTrustedTimeAvailableForCleanup())
    return;

  CleanupFiles();
  forecast_cleanup_pending = false;
}

void
SkySightManager::CleanupFiles() noexcept
{
  SkySightCache::Cleanup(GetCachePath());
}

std::size_t
SkySightManager::NumLayers() const noexcept
{
  return api->NumLayers();
}

const SkySight::Layer *
SkySightManager::GetLayer(std::size_t index) const noexcept
{
  return api->GetLayer(index);
}

const std::vector<SkySightRegionEntry> &
SkySightManager::GetRegions() const noexcept
{
  return api->GetRegions();
}

std::string_view
SkySightManager::GetRegion() const noexcept
{
  return api->GetRegion();
}

std::size_t
SkySightManager::NumSelectedLayers() const noexcept
{
  return api->NumSelectedLayers();
}

const SkySight::Layer *
SkySightManager::GetSelectedLayer(std::size_t index) const noexcept
{
  return api->GetSelectedLayer(index);
}

const SkySight::Layer *
SkySightManager::GetSelectedLayer(std::string_view id) const noexcept
{
  return const_cast<SkySightAPI &>(*api).GetSelectedLayer(id);
}

bool
SkySightManager::IsSelectedLayer(std::string_view id) const noexcept
{
  return api->IsSelectedLayer(id);
}

bool
SkySightManager::SelectedLayersFull() const noexcept
{
  return api->SelectedLayersFull();
}

bool
SkySightManager::HasCredentials() const noexcept
{
  return api->HasCredentials();
}

bool
SkySightManager::IsThrottled() const noexcept
{
  return api->IsThrottled();
}

bool
SkySightManager::IsSuspendedForSession() const noexcept
{
  return api->IsSuspendedForSession();
}

time_t
SkySightManager::GetThrottleRemainingSeconds() const noexcept
{
  return api->GetThrottleRemainingSeconds();
}

time_t
SkySightManager::GetDatafilesRetryRemainingSeconds() const noexcept
{
  return api->GetDatafilesRetryRemainingSeconds();
}

std::string_view
SkySightManager::GetActiveLayerId() const noexcept
{
  return active_layer != nullptr
    ? std::string_view{active_layer->id}
    : std::string_view{};
}

std::string_view
SkySightManager::GetDisplayedLayerId() const noexcept
{
  return displayed_layer != nullptr
    ? std::string_view{displayed_layer->id}
    : std::string_view{};
}

StaticString<128>
SkySightManager::GetOverlayLabel() const noexcept
{
  StaticString<128> label;
  label = "SkySight";

  const auto *layer = active_layer != nullptr
    ? active_layer
    : displayed_layer;
  if (layer == nullptr)
    return label;

  label.AppendFormat(" %s", layer->name.c_str());

  if (!layer->SupportsLiveTiles() && layer->forecast_time > 0) {
    const auto time_label =
      FormatLocalTimeHHMM(TimeStamp(std::chrono::duration<double>(layer->forecast_time)),
                          CommonInterface::GetComputerSettings().utc_offset);
    label.AppendFormat(" %s", time_label.c_str());
  }

  return label;
}

bool
SkySightManager::AddSelectedLayer(std::string_view id)
{
  return AddSelectedLayer(id, true);
}

bool
SkySightManager::AddSelectedLayer(std::string_view id, bool save_profile)
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
    selected.datafiles_pending = true;
    selected.updating = selected.ShouldShowUpdating();
  }

  if (!api->AddSelectedLayer(selected))
    return false;

  if (save_profile) {
    SaveSelectedLayers();
    api->PollSelectedDatafiles();
  }

  return true;
}

bool
SkySightManager::RemoveSelectedLayer(std::string_view id)
{
  if (!api->RemoveSelectedLayer(id))
    return false;

  SaveSelectedLayers();
  return true;
}

bool
SkySightManager::HasForecastLayers() const noexcept
{
  for (std::size_t i = 0; i < api->NumLayers(); ++i) {
    const auto *layer = api->GetLayer(i);
    if (layer != nullptr && !layer->SupportsLiveTiles())
      return true;
  }

  return false;
}

bool
SkySightManager::IsForecastDecodeAvailable() const noexcept
{
  return SkySightFileDecoder::IsNetCdfDecodeAvailable();
}

void
SkySightManager::RefreshCatalog() noexcept
{
  MaybeCleanupFiles();
  api->PollRegions();
  api->PollLayers();
}

void
SkySightManager::PollPendingDatafiles() noexcept
{
  MaybeCleanupFiles();
  api->Poll();
}

bool
SkySightManager::SelectForecastTime(std::string_view id, time_t forecast_time)
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
SkySightManager::SelectAutomaticForecastTime(std::string_view id)
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
SkySightManager::SelectPageLayer(std::string_view id)
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
SkySightManager::PreloadForecast(std::string_view id) noexcept
{
  return api->PreloadDatafiles(id);
}

bool
SkySightManager::PreloadAllForecasts() noexcept
{
  return api->PreloadAllDatafiles();
}

unsigned
SkySightManager::GetPreloadFileCount() const noexcept
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
SkySightManager::GetSelectedForecastLayerCount() const noexcept
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
SkySightManager::ReloadSelectedLayersFromProfile()
{
  api->ClearSelectedLayers();

  const char *configured_layers = Profile::Get(ProfileKeys::SkySightSelectedLayers);
  if (configured_layers == nullptr || *configured_layers == '\0')
    /* Accept pre-rename profile keys from early SkySight builds. */
    configured_layers = Profile::Get("SkysightSelectedLayers");
  if (configured_layers == nullptr || *configured_layers == '\0')
    return;

  std::string remaining{configured_layers};
  while (!remaining.empty()) {
    const auto split = remaining.find(',');
    const auto layer_id = remaining.substr(0, split);
    if (!layer_id.empty())
      (void)AddSelectedLayer(layer_id, false);

    if (split == std::string::npos)
      break;

    remaining.erase(0, split + 1);
  }

  api->PollSelectedDatafiles();
}

void
SkySightManager::SaveSelectedLayers() const
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
SkySightManager::OnLayerCatalogChanged(std::string_view active_id,
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
SkySightManager::ResetTiles() noexcept
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
SkySightManager::SetLayerActive(std::string_view id)
{
  auto *layer = api->GetLayer(id);
  if (layer == nullptr)
    return false;

  if (!api->IsSelectedLayer(id) && !AddSelectedLayer(id))
    return false;

  if (active_layer != layer)
    api->CancelTileDownloads();

  active_layer = layer;
  if (active_layer->SupportsLiveTiles()) {
    active_layer->last_update = 0;
    api->ResetLastUpdates();
  } else {
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
SkySightManager::ApplyPageOverlay(std::string_view overlay_id,
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
SkySightManager::DeactivateLayer()
{
  api->CancelTileDownloads();
  active_layer = nullptr;
  ResetTiles();
  OnDataUpdated();
}

void
SkySightManager::OnDataUpdated() noexcept
{
  MaybeCleanupFiles();
  forecast_image_dirty = true;

  if (auto *map = UIGlobals::GetMapIfActive())
    map->DeferRedraw();

  if (CommonInterface::main_window != nullptr)
    CommonInterface::main_window->SendCalculatedUpdate();
}

void
SkySightManager::OnForecastThrottled() noexcept
{
  if (throttle_notification_active)
    return;

  throttle_notification_active = true;
  if (IsSuspendedForSession()) {
    Message::AddMessage(_("SkySight request limit reached; downloads paused "
                          "until restart or settings reload."));
    return;
  }

  StaticString<128> message;
  message.Format(_("SkySight rate limit reached; continuing in %u seconds."),
                 unsigned(GetThrottleRemainingSeconds()));
  Message::AddMessage(message.c_str());
}

void
SkySightManager::OnForecastResumed() noexcept
{
  if (!throttle_notification_active)
    return;

  throttle_notification_active = false;
  Message::AddMessage(_("SkySight downloads resumed."));
}

void
SkySightManager::OnForecastProgressCancelled() noexcept
{
  if (!forecast_progress_visible)
    return;

  BackgroundDownloadProgress::Get().End();
  forecast_progress_visible = false;
}

void
SkySightManager::OnForecastProgress(const SkySight::ForecastProgress &progress) noexcept
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
    if (progress.retry_seconds > 0)
      text.Format(_("SkySight rate limited; continuing in %u seconds..."),
                  progress.retry_seconds);
    else
      text = _("SkySight request limit reached; downloads paused for this session.");
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
SkySightManager::UpdateActiveLayer(unsigned index, Path path,
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
SkySightManager::DisplayForecastLayer()
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
SkySightManager::DisplayTileLayer()
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

  const time_t refresh_time = active_layer->last_update != 0
    ? active_layer->last_update
    : (std::time(nullptr) / 600) * 600;
  const int tiles_per_axis = 1 << base_tile.zoom;
  const auto normalize_x = [tiles_per_axis](int value) {
    int result = value % tiles_per_axis;
    if (result < 0)
      result += tiles_per_axis;

    return (uint32_t)result;
  };

  bool any_visible = false;
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

      GeoBitmap::TileData tile{base_tile.zoom, normalize_x(x), (uint32_t)y};

      if (!GeoBitmap::GetBounds(tile).Overlaps(map_bounds)) {
        map_window->SetOverlay(slot, nullptr);
        tile_filenames[slot].clear();
        continue;
      }

      any_visible = true;
      bool found = false;
      for (unsigned step = 0; step < 3; ++step) {
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

      if (!found) {
        api->EnsureTile(*active_layer, refresh_time, tile);
        map_window->SetOverlay(slot, nullptr);
        tile_filenames[slot].clear();
      }
    }
  }

  return any_visible;
#endif
}

void
SkySightManager::Render()
{
#ifndef ENABLE_OPENGL
  /* Forecast and live-tile overlays are OpenGL-only.  Avoid reading
     shared SkySight state from the DrawThread on other backends. */
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
