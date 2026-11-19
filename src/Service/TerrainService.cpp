// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TerrainService.hpp"
#include "Terrain/CogTerrain.hpp"
#include "Terrain/RasterTerrain.hpp"  // For XcmTerrain
#include "Operation/Operation.hpp"
#include "LogFile.hpp"

namespace Service {

TerrainService::TerrainService(Event::EventBus &_bus, FileCache *_file_cache)
  : StandbyThread("TerrainService"),
    bus(_bus),
    file_cache(_file_cache)
{
  view_subscription = bus.Subscribe<Event::ViewChangedEvent>(
    [this](const Event::ViewChangedEvent &e) { OnViewChanged(e); });

  prefetch_subscription = bus.Subscribe<Event::TerrainPrefetchRequestEvent>(
    [this](const Event::TerrainPrefetchRequestEvent &e) { OnPrefetchRequest(e); });
}

TerrainService::~TerrainService() noexcept
{
  Stop();
}

void
TerrainService::Start(const char *terrain_file)
{
  if (terrain_file == nullptr || *terrain_file == '\0')
    return;

  terrain_path = Path(terrain_file);
  Start(terrain_path);
}

void
TerrainService::Start(Path path)
{
  if (path == nullptr)
    return;

  LogFormat("TerrainService: Starting load of %s", path.c_str());

  terrain_loading = true;
  terrain_path = path;

  QueueWork({WorkType::LOAD_TERRAIN});
}

TerrainHeight
TerrainService::GetHeight(GeoPoint location) const noexcept
{
  if (terrain == nullptr)
    return TerrainHeight::Invalid();

  return terrain->GetTerrainHeight(location, 1);
}

void
TerrainService::Stop() noexcept
{
  view_subscription.Unsubscribe();
  prefetch_subscription.Unsubscribe();
  LockStop();
  terrain.reset();
}

void
TerrainService::OnViewChanged(const Event::ViewChangedEvent &event)
{
  if (!IsAutoTileMode())
    return;

  if (!event.center.IsValid())
    return;

  if (terrain != nullptr) {
    if (terrain->UpdateTiles(event.center, event.radius_meters)) {
      QueueWork({WorkType::PROCESS_TILE_QUEUE});
    }
  }
}

void
TerrainService::OnPrefetchRequest(const Event::TerrainPrefetchRequestEvent &event)
{
  if (!IsAutoTileMode())
    return;

  if (!event.bounds.IsValid())
    return;

  const GeoPoint center = event.bounds.GetCenter();
  const double geo_width = event.bounds.GetGeoWidth();
  const double radius = geo_width > 0 ? geo_width / 2 : 50000;

  if (terrain != nullptr) {
    if (terrain->UpdateTiles(center, radius)) {
      QueueWork({WorkType::PROCESS_TILE_QUEUE});
    }
  }
}

void
TerrainService::QueueWork(WorkItem item)
{
  const std::lock_guard lock{mutex};
  work_queue.push(std::move(item));
  Trigger();
}

void
TerrainService::Tick() noexcept
{
  WorkItem item;

  {
    const std::lock_guard lock{mutex};
    if (work_queue.empty())
      return;

    item = work_queue.front();
    work_queue.pop();
  }

  mutex.unlock();

  switch (item.type) {
  case WorkType::LOAD_TERRAIN:
    DoLoadTerrain();
    break;

  case WorkType::PROCESS_TILE_QUEUE:
    DoProcessTileQueue();
    break;

  case WorkType::LOAD_INTERNAL_TILE:
    DoLoadInternalTile(item);
    break;
  }

  mutex.lock();

  if (!work_queue.empty())
    Trigger();
}

void
TerrainService::DoLoadTerrain()
{
  LogFormat("TerrainService: Loading terrain from %s", terrain_path.c_str());

  try {
    NullOperationEnvironment operation;

    const char *path_str = terrain_path.c_str();

    if (CogTerrain::IsAutoTileProvider(path_str)) {
      if (curl != nullptr) {
        terrain = CogTerrain::OpenAutoTile(*curl, path_str);
        LogString("TerrainService: Opened auto-tile terrain");

        bus.PostToMain(Event::TerrainReadyEvent{});
      } else {
        LogString("TerrainService: No curl instance for auto-tile terrain");
      }
    } else if (CogTerrain::IsCogTerrain(path_str)) {
      terrain = CogTerrain::OpenTerrain(file_cache, curl, path_str, operation);
      LogFormat("TerrainService: Loaded COG terrain from %s", path_str);

      if (terrain != nullptr && terrain->IsDefined()) {
        const auto bounds = terrain->GetBounds();
        bus.PostToMain(Event::TerrainTileLoadedEvent{
          CogTileId::FromLocation(bounds.GetCenter()),
          bounds
        });
      }
    } else {
      terrain = XcmTerrain::OpenTerrain(file_cache, terrain_path, operation);
      LogFormat("TerrainService: Loaded raster terrain from %s", path_str);

      if (terrain != nullptr && terrain->IsDefined()) {
        const auto bounds = terrain->GetBounds();
        bus.PostToMain(Event::TerrainTileLoadedEvent{
          CogTileId::FromLocation(bounds.GetCenter()),
          bounds
        });
      }
    }

    terrain_loading = false;

  } catch (...) {
    LogError(std::current_exception(), "TerrainService: Failed to load terrain");
    terrain_loading = false;

    bus.PostToMain(Event::TerrainTileFailedEvent{
      CogTileId{0, 0},
      "Failed to load terrain"
    });
  }
}

void
TerrainService::DoProcessTileQueue()
{
  if (terrain == nullptr || !terrain->IsAutoTileMode())
    return;

  auto *cog_terrain = dynamic_cast<CogTerrain *>(terrain.get());
  if (cog_terrain == nullptr)
    return;

  while (cog_terrain->ProcessPendingTile()) {
    const std::lock_guard lock{mutex};
    if (IsStopped())
      break;
  }

  if (cog_terrain->IsDefined()) {
    const auto bounds = cog_terrain->GetBounds();
    bus.PostToMain(Event::TerrainTileLoadedEvent{
      CogTileId::FromLocation(bounds.GetCenter()),
      bounds
    });
  }
}

void
TerrainService::DoLoadInternalTile(const WorkItem &item)
{
  if (terrain == nullptr || !terrain->IsAutoTileMode())
    return;

  auto *cog_terrain = dynamic_cast<CogTerrain *>(terrain.get());
  if (cog_terrain == nullptr)
    return;

  bus.PostToMain(Event::TerrainInternalTileLoadedEvent{
    item.tile_id,
    item.level,
    item.tx,
    item.ty
  });
}

} // namespace Service
