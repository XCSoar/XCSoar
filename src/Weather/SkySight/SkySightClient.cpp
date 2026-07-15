// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SkySightClient.hpp"
#include "SkySightCache.hpp"
#include "SkySightAPI.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "UIGlobals.hpp"
#include "LocalPath.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "MapWindow/OverlayBitmap.hpp"
#include "system/FileUtil.hpp"

#include <chrono>

namespace {

class OlderThanFileVisitor final : public File::Visitor {
  const std::chrono::system_clock::time_point cutoff;

public:
  explicit OlderThanFileVisitor(std::chrono::system_clock::time_point _cutoff) noexcept
    :cutoff(_cutoff) {}

  void Visit(Path full_path, [[maybe_unused]] Path filename) override {
    if (File::GetLastModification(full_path) < cutoff)
      File::Delete(full_path);
  }
};

} // namespace

SkySightClient::SkySightClient(CurlGlobal &curl)
  :api(std::make_unique<SkySightAPI>(*this, curl, GetLocalPath()))
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

  ResetTiles();
  active_layer = nullptr;

  const auto &settings = CommonInterface::GetComputerSettings().weather.skysight;
  api->Configure(settings.email.c_str(), settings.password.c_str(),
                 settings.region.c_str());
  ReloadSelectedLayersFromProfile();
  api->PollRegions();
  api->PollLayers();

  const char *configured_layer = Profile::Get(ProfileKeys::WeatherLayerDisplayed);
  if (configured_layer != nullptr && !std::string_view{configured_layer}.empty())
    (void)SetLayerActive(configured_layer);
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

  const auto *layer = api->GetLayer(id);
  if (layer == nullptr)
    return false;

  auto selected = *layer;
  if (!selected.SupportsLiveTiles())
    selected.updating = request_datafiles;

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

  displayed_layer = nullptr;
  displayed_zoom = 0;
}

bool
SkySightClient::SetLayerActive(std::string_view id)
{
  auto *layer = api->GetLayer(id);
  if (layer == nullptr || !layer->SupportsLiveTiles())
    return false;

  active_layer = layer;
  Profile::Set(ProfileKeys::WeatherLayerDisplayed, layer->id.c_str());
  ResetTiles();
  OnDataUpdated();
  return true;
}

void
SkySightClient::DeactivateLayer()
{
  active_layer = nullptr;
  Profile::Set(ProfileKeys::WeatherLayerDisplayed, "");
  ResetTiles();
  OnDataUpdated();
}

void
SkySightClient::OnDataUpdated() noexcept
{
  if (auto *map = UIGlobals::GetMapIfActive())
    map->DeferRedraw();

  if (CommonInterface::main_window != nullptr)
    CommonInterface::main_window->SendCalculatedUpdate();
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

  char label[128];
  std::snprintf(label, sizeof(label), "SkySight: %s (%u/%u/%u)",
                active_layer->name.c_str(), tile.zoom, tile.x, tile.y);
  bitmap->SetLabel(label);

  map->SetOverlay(index, std::move(bitmap));
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
  bool any_visible = false;
  bool probe_queued = false;
  unsigned slot = 0;
  for (int x = int(base_tile.x) - 1; x <= int(base_tile.x) + 1; ++x) {
    for (int y = int(base_tile.y) - 1; y <= int(base_tile.y) + 1; ++y, ++slot) {
      GeoBitmap::TileData tile{base_tile.zoom, (uint16_t)x, (uint16_t)y};

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
  if (active_layer != nullptr)
    (void)DisplayTileLayer();
}
