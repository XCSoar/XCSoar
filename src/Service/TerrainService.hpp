// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Event/EventBus.hpp"
#include "Event/TerrainEvents.hpp"
#include "Terrain/TerrainBase.hpp"
#include "Terrain/CogTileManager.hpp"
#include "thread/StandbyThread.hpp"
#include "io/FileCache.hpp"

#include <memory>
#include <queue>

class CurlGlobal;
class Path;

namespace Service {

/**
 * Manages terrain data loading and provides non-blocking terrain queries.
 *
 * The service:
 * - Loads terrain files asynchronously
 * - Manages COG tile downloading in the background
 * - Provides non-blocking GetHeight() that returns Invalid for uncached data
 * - Posts events when terrain becomes available
 *
 * Consumers subscribe to TerrainTileLoadedEvent to know when to re-render
 * or recalculate elevations.
 */
class TerrainService : private StandbyThread {
public:
  /**
   * Work items for the background thread.
   */
  enum class WorkType {
    LOAD_TERRAIN,       // Initial terrain file load
    PROCESS_TILE_QUEUE, // Process pending COG tile requests
    LOAD_INTERNAL_TILE, // Load a specific internal tile
  };

  struct WorkItem {
    WorkType type;
    CogTileId tile_id{0, 0};
    unsigned level = 0;
    unsigned tx = 0, ty = 0;
  };

private:
  Event::EventBus &bus;
  FileCache *file_cache;
  CurlGlobal *curl = nullptr;

  std::unique_ptr<TerrainBase> terrain;
  Event::EventBus::Subscription view_subscription;
  Event::EventBus::Subscription prefetch_subscription;

  std::queue<WorkItem> work_queue;
  bool terrain_loading = false;
  AllocatedPath terrain_path{nullptr};

public:
  explicit TerrainService(Event::EventBus &_bus, FileCache *_file_cache);
  ~TerrainService() noexcept;

  /**
   * Set the curl instance for HTTP terrain loading.
   * Must be called before Start() if using COG terrain.
   */
  void SetCurl(CurlGlobal *_curl) noexcept { curl = _curl; }

  /**
   * Start loading terrain from the given file/URL.
   * Non-blocking - returns immediately, posts events when ready.
   */
  void Start(const char *terrain_file);

  /**
   * Start loading terrain from a Path.
   */
  void Start(Path path);

  /**
   * Get the terrain height at a location.
   * Non-blocking - returns Invalid if data is not cached.
   */
  [[nodiscard]]
  TerrainHeight GetHeight(GeoPoint location) const noexcept;

  /**
   * Get the underlying terrain object (may be null).
   */
  [[nodiscard]]
  TerrainBase *GetTerrain() noexcept { return terrain.get(); }

  [[nodiscard]]
  const TerrainBase *GetTerrain() const noexcept { return terrain.get(); }

  /**
   * Check if terrain is defined and usable.
   */
  [[nodiscard]]
  bool IsDefined() const noexcept {
    return terrain != nullptr && terrain->IsDefined();
  }

  /**
   * Check if this is auto-tile COG terrain.
   */
  [[nodiscard]]
  bool IsAutoTileMode() const noexcept {
    return terrain != nullptr && terrain->IsAutoTileMode();
  }

  /**
   * Stop the service and release resources.
   */
  void Stop() noexcept;

private:
  void OnViewChanged(const Event::ViewChangedEvent &event);
  void OnPrefetchRequest(const Event::TerrainPrefetchRequestEvent &event);

  void QueueWork(WorkItem item);

  void Tick() noexcept override;
  void DoLoadTerrain();
  void DoProcessTileQueue();
  void DoLoadInternalTile(const WorkItem &item);
};

} // namespace Service
