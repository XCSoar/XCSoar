// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Skysight.hpp"
#include "SkysightCache.hpp"
#include "SkysightAPI.hpp"
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

Skysight::Skysight(CurlGlobal &curl)
  :api(std::make_unique<SkysightAPI>(*this, curl, GetLocalPath()))
{
  Init();
}

Skysight::~Skysight() = default;

AllocatedPath
Skysight::GetLocalPath() noexcept
{
  return MakeCacheDirectory("skysight");
}

void
Skysight::Init()
{
  CleanupFiles();

  ResetTiles();
  active_layer = nullptr;

  const auto &settings = CommonInterface::GetComputerSettings().weather.skysight;
  api->Configure(settings.email.c_str(), settings.password.c_str(),
                 settings.region.c_str());

  const char *configured_layer = Profile::Get(ProfileKeys::WeatherLayerDisplayed);
  if (configured_layer != nullptr && !std::string_view{configured_layer}.empty())
    (void)SetLayerActive(configured_layer);
}

void
Skysight::CleanupFiles() noexcept
{
  SkysightCache::Cleanup(GetLocalPath());
}

std::size_t
Skysight::NumLayers() const noexcept
{
  return api->NumLayers();
}

const SkySight::Layer *
Skysight::GetLayer(std::size_t index) const noexcept
{
  return api->GetLayer(index);
}

bool
Skysight::HasCredentials() const noexcept
{
  return api->HasCredentials();
}

std::string_view
Skysight::GetActiveLayerId() const noexcept
{
  return active_layer != nullptr
    ? std::string_view{active_layer->id}
    : std::string_view{};
}

void
Skysight::ResetTiles() noexcept
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
Skysight::SetLayerActive(std::string_view id)
{
  auto *layer = api->GetLayer(id);
  if (layer == nullptr)
    return false;

  active_layer = layer;
  active_layer->last_update = 0;
  api->ResetLastUpdates();
  Profile::Set(ProfileKeys::WeatherLayerDisplayed, layer->id.c_str());
  ResetTiles();
  OnDataUpdated();
  return true;
}

void
Skysight::DeactivateLayer()
{
  active_layer = nullptr;
  Profile::Set(ProfileKeys::WeatherLayerDisplayed, "");
  ResetTiles();
  OnDataUpdated();
}

void
Skysight::OnDataUpdated() noexcept
{
  if (auto *map = UIGlobals::GetMapIfActive())
    map->DeferRedraw();

  if (CommonInterface::main_window != nullptr)
    CommonInterface::main_window->SendCalculatedUpdate();
}

bool
Skysight::UpdateActiveLayer(unsigned index, Path path,
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
Skysight::DisplayTileLayer()
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
  bool any_visible = false;
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
Skysight::Render()
{
  if (active_layer != nullptr)
    (void)DisplayTileLayer();
}