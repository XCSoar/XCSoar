// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CogTerrain.hpp"
#include "RasterTileCache.hpp"
#include "RasterTraits.hpp"
#include "Operation/Operation.hpp"
#include "system/Path.hpp"
#include "LocalPath.hpp"
#include "LogFile.hpp"

#ifdef HAVE_HTTP
#include "lib/curl/Global.hxx"
#endif

#include <cstring>
#include <stdexcept>

void
CogTerrain::Load(OperationEnvironment &operation)
{
  operation.SetProgressRange(100);
  operation.SetProgressPosition(10);

  const UnsignedPoint2D size = loader.GetSize();
  if (size.x == 0 || size.y == 0)
    throw std::runtime_error("COG terrain has zero size");

  const GeoBounds &bounds = loader.GetBounds();
  if (!bounds.IsValid())
    throw std::runtime_error("COG terrain has no valid geographic bounds");

  auto &cache = map.GetTileCache();
  cache.SetBounds(bounds);

  operation.SetProgressPosition(30);

  const unsigned level_count = loader.GetLevelCount();
  if (level_count == 0)
    throw std::runtime_error("COG has no pyramid levels");

  const CogLevel &full_level = loader.GetLevel(0);
  const UnsignedPoint2D tile_size = loader.GetTileSize(0);

  cache.SetSize({size.x, size.y},
                {static_cast<uint_least16_t>(tile_size.x),
                 static_cast<uint_least16_t>(tile_size.y)},
                {full_level.tiles_x, full_level.tiles_y});

  operation.SetProgressPosition(50);

  map.UpdateProjection();

  operation.SetProgressPosition(70);

  LogFmt("COG terrain loaded: {}x{} pixels, {} levels, {}x{} tiles, bounds: {:.4f},{:.4f} to {:.4f},{:.4f}",
         size.x, size.y, level_count,
         full_level.tiles_x, full_level.tiles_y,
         bounds.GetWest().Degrees(), bounds.GetSouth().Degrees(),
         bounds.GetEast().Degrees(), bounds.GetNorth().Degrees());

  operation.SetProgressPosition(100);
}

std::unique_ptr<CogTerrain>
CogTerrain::OpenTerrain([[maybe_unused]] FileCache *cache,
                        Path path,
                        OperationEnvironment &operation)
{
  auto terrain = std::make_unique<CogTerrain>();

  terrain->source_path = path.c_str();

  if (!terrain->loader.Open(path))
    throw std::runtime_error("Failed to open COG terrain file");

  terrain->Load(operation);

  return terrain;
}

#ifdef HAVE_HTTP

std::unique_ptr<CogTerrain>
CogTerrain::OpenTerrainHttp(CurlGlobal &curl,
                            const char *url,
                            OperationEnvironment &operation)
{
  auto terrain = std::make_unique<CogTerrain>();

  terrain->curl = &curl;
  terrain->source_path = url;

  if (!terrain->loader.OpenHttp(curl, url))
    throw std::runtime_error("Failed to open COG terrain from HTTP");

  terrain->Load(operation);

  return terrain;
}

#else

std::unique_ptr<CogTerrain>
CogTerrain::OpenTerrainHttp([[maybe_unused]] CurlGlobal &curl,
                            [[maybe_unused]] const char *url,
                            [[maybe_unused]] OperationEnvironment &operation)
{
  throw std::runtime_error("HTTP terrain loading not available");
}

#endif

std::unique_ptr<CogTerrain>
CogTerrain::OpenTerrain([[maybe_unused]] FileCache *cache,
                        [[maybe_unused]] CurlGlobal *curl,
                        const char *path_or_url,
                        OperationEnvironment &operation)
{
  if (path_or_url == nullptr || *path_or_url == '\0')
    throw std::runtime_error("No terrain path specified");

  if (IsHttpUrl(path_or_url)) {
#ifdef HAVE_HTTP
    if (curl == nullptr)
      throw std::runtime_error("HTTP terrain requires curl");
    return OpenTerrainHttp(*curl, path_or_url, operation);
#else
    throw std::runtime_error("HTTP terrain not supported in this build");
#endif
  }

  return OpenTerrain(cache, Path(path_or_url), operation);
}

bool
CogTerrain::IsAutoTileProvider(const char *name) noexcept
{
  if (name == nullptr || *name == '\0')
    return false;

  return strcmp(name, "srtm_gl1") == 0 ||
         strcmp(name, "srtm_gl3") == 0 ||
         strcmp(name, "SRTM_GL1") == 0 ||
         strcmp(name, "SRTM_GL3") == 0;
}

#ifdef HAVE_HTTP

std::unique_ptr<CogTerrain>
CogTerrain::OpenAutoTile(CurlGlobal &curl, const char *provider)
{
  auto terrain = std::make_unique<CogTerrain>();

  terrain->curl = &curl;
  terrain->source_path = provider;
  terrain->auto_tile_mode = true;

  auto cache_path = GetCachePath();
  if (cache_path != nullptr) {
    auto terrain_cache = AllocatedPath::Build(cache_path, "terrain");
    terrain->tile_manager.SetCachePath(terrain_cache.c_str());
    LogFormat("COG terrain cache: %s", terrain_cache.c_str());
  }

  auto p = CogTileManager::ParseProvider(provider);
  terrain->tile_manager.SetProvider(p, curl);

  LogFormat("COG auto-tile terrain initialized with provider: %s", provider);

  return terrain;
}

#else

std::unique_ptr<CogTerrain>
CogTerrain::OpenAutoTile([[maybe_unused]] CurlGlobal &curl,
                         [[maybe_unused]] const char *provider)
{
  throw std::runtime_error("Auto-tile terrain not available (no HTTP support)");
}

#endif

TerrainHeight
CogTerrain::GetTerrainHeight(const GeoPoint location,
                             unsigned quantisation) const noexcept
{
  if (auto_tile_mode)
    return tile_manager.GetHeight(location, quantisation);

  Lease lease(*this);
  return lease->GetHeight(location);
}

bool
CogTerrain::UpdateTiles(const GeoPoint &location, double radius) noexcept
{
  if (auto_tile_mode) {
    if (!location.IsValid())
      return false;

    bool has_pending = tile_manager.UpdateTiles(location, radius);

    if (has_pending)
      tile_manager.ProcessOneRequest();

    if (tile_manager.HasPendingInternalTiles())
      tile_manager.ProcessInternalTiles();

    return tile_manager.HasPendingTiles() ||
           tile_manager.HasPendingInternalTiles();
  }

  auto &tile_cache = map.GetTileCache();
  if (!tile_cache.IsValid())
    return false;

  return map.IsDirty();
}

bool
CogTerrain::ProcessPendingTile() noexcept
{
  if (!auto_tile_mode)
    return false;

  if (tile_manager.ProcessOneRequest())
    return true;

  return tile_manager.ProcessInternalTiles();
}
