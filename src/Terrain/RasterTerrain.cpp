/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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

static const TCHAR *const terrain_cache_name = _T("terrain");

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

inline void
RasterTerrain::SaveCache(FileCache &cache, Path path) const
{
  auto os = cache.Save(terrain_cache_name, path);
  BufferedOutputStream bos(*os);
  map.SaveCache(bos);
  bos.Flush();
  os->Commit();
}

inline bool
RasterTerrain::Load(Path path, FileCache *cache,
                    OperationEnvironment &operation)
{
  try {
    if (LoadCache(cache, path))
      return true;
  } catch (...) {
    LogError(std::current_exception(), "Failed to load terrain cache");
  }

  if (!LoadTerrainOverview(archive.get(), map.GetTileCache(), operation))
    return false;

  map.UpdateProjection();

  if (cache != nullptr) {
    try {
      SaveCache(*cache, path);
    } catch (...) {
      LogError(std::current_exception(), "Failed to save terrain cache");
    }
  }

  return true;
}

RasterTerrain *
RasterTerrain::OpenTerrain(FileCache *cache, OperationEnvironment &operation)
try {
  const auto path = Profile::GetPath(ProfileKeys::MapFile);
  if (path.IsNull())
    return nullptr;

  RasterTerrain *rt = new RasterTerrain(ZipArchive(path));
  if (!rt->Load(path, cache, operation)) {
    delete rt;
    return nullptr;
  }

  return rt;
} catch (const std::runtime_error &e) {
  operation.SetErrorMessage(UTF8ToWideConverter(e.what()));
  return nullptr;
}

bool
RasterTerrain::UpdateTiles(const GeoPoint &location, double radius)
{
  auto &tile_cache = map.GetTileCache();
  if (!tile_cache.IsValid())
    return false;

  UpdateTerrainTiles(archive.get(), tile_cache, mutex,
                     map.GetProjection(), location, radius);
  return map.IsDirty();
}
