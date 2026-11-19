// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TerrainBase.hpp"
#include "CogTerrain.hpp"
#include "RasterTerrain.hpp"
#include "CogLoader.hpp"
#include "Profile/Profile.hpp"
#include "Operation/Operation.hpp"
#include "system/Path.hpp"
#include "LogFile.hpp"

#include <type_traits>

static_assert(std::is_base_of_v<TerrainBase, XcmTerrain>);
static_assert(std::is_base_of_v<TerrainBase, CogTerrain>);

#ifdef HAVE_HTTP
#include "net/http/Init.hpp"
#endif

std::unique_ptr<TerrainBase>
TerrainBase::OpenTerrain(FileCache *cache, OperationEnvironment &operation)
try {
  const auto terrain_file = Profile::Get(ProfileKeys::TerrainFile);
  if (terrain_file != nullptr && *terrain_file != '\0') {
    LogFormat("Opening terrain: %s", terrain_file);

    if (CogTerrain::IsAutoTileProvider(terrain_file)) {
#ifdef HAVE_HTTP
      if (Net::curl != nullptr) {
        LogFormat("Using auto-tile terrain provider: %s", terrain_file);
        return CogTerrain::OpenAutoTile(*Net::curl, terrain_file);
      } else {
        LogFormat("Auto-tile terrain requested but curl not available");
      }
#else
      LogFormat("Auto-tile terrain not supported in this build (no HTTP)");
#endif
    } else if (CogLoader::IsHttpUrl(terrain_file)) {
#ifdef HAVE_HTTP
      if (Net::curl != nullptr) {
        return CogTerrain::OpenTerrainHttp(*Net::curl, terrain_file, operation);
      } else {
        LogFormat("HTTP terrain requested but curl not available");
      }
#else
      LogFormat("HTTP terrain not supported in this build");
#endif
    } else {
      return CogTerrain::OpenTerrain(cache, Path(terrain_file), operation);
    }
  }

  const auto map_path = Profile::GetPath(ProfileKeys::MapFile);
  if (map_path == nullptr)
    return nullptr;

  return XcmTerrain::OpenTerrain(cache, map_path, operation);
} catch (...) {
  operation.SetError(std::current_exception());
  return nullptr;
}
