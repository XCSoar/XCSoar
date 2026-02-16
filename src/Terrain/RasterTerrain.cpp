// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RasterTerrain.hpp"
#include "Loader.hpp"
#include "Profile/Profile.hpp"
#include "io/ZipArchive.hpp"
#include "io/FileCache.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "io/Reader.hxx"
#include "io/BufferedReader.hxx"
#include "system/ConvertPathName.hpp"
#include "Operation/Operation.hpp"
#include "util/ConvertString.hpp"
#include "LogFile.hpp"

static const char *const terrain_cache_name = _T("terrain");

inline bool
RasterTerrain::LoadCache(FileCache &cache, Path path)
{
  auto r = cache.Load(terrain_cache_name, path);
  if (!r)
    return false;

  BufferedReader br(*r);
  map.LoadCache(br);
  return true;
}

inline bool
RasterTerrain::LoadCache(FileCache *cache, Path path)
{
  return cache != nullptr && LoadCache(*cache, path);
}

inline void
RasterTerrain::SaveCache(FileCache &cache, Path path) const
{
  auto os = cache.Save(terrain_cache_name, path);
  BufferedOutputStream bos(*os);
  map.SaveCache(bos);
  bos.Flush();
  os->Commit();
}

inline void
RasterTerrain::Load(Path path, FileCache *cache,
                    OperationEnvironment &operation)
{
  try {
    if (LoadCache(cache, path))
      return;
  } catch (...) {
    LogError(std::current_exception(), "Failed to load terrain cache");
  }

  LoadTerrainOverview(archive.get(), map.GetTileCache(), operation);

  map.UpdateProjection();

  if (cache != nullptr) {
    try {
      SaveCache(*cache, path);
    } catch (...) {
      LogError(std::current_exception(), "Failed to save terrain cache");
    }
  }
}

std::unique_ptr<RasterTerrain>
RasterTerrain::OpenTerrain(FileCache *cache, Path path,
                           OperationEnvironment &operation)
{
  auto rt = std::make_unique<RasterTerrain>(ZipArchive{path});
  rt->Load(path, cache, operation);
  return rt;
}

std::unique_ptr<RasterTerrain>
RasterTerrain::OpenTerrain(FileCache *cache, OperationEnvironment &operation)
try {
  const auto path = Profile::GetPath(ProfileKeys::MapFile);
  if (path == nullptr)
    return nullptr;

  return OpenTerrain(cache, path, operation);
} catch (...) {
  operation.SetError(std::current_exception());
  return nullptr;
}

bool
RasterTerrain::UpdateTiles(const GeoPoint &location, double radius) noexcept
{
  auto &tile_cache = map.GetTileCache();
  if (!tile_cache.IsValid())
    return false;

  try {
    UpdateTerrainTiles(archive.get(), tile_cache, mutex,
                       map.GetProjection(), location, radius);
  } catch (...) {
    LogError(std::current_exception(), "Failed to update terrain tiles");
  }

  return map.IsDirty();
}
