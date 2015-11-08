/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "IO/FileCache.hpp"
#include "Compatibility/path.h"
#include "OS/ConvertPathName.hpp"

#include <zzip/zzip.h>

#include <windef.h> /* for MAX_PATH */

#include <string.h>

static const TCHAR *const terrain_cache_name = _T("terrain");

RasterTerrain::~RasterTerrain()
{
    zzip_dir_close(dir);
}

inline bool
RasterTerrain::LoadCache(FileCache &cache, Path path)
{
  bool success = false;

  FILE *file = cache.Load(terrain_cache_name, path);
  if (file != nullptr) {
    success = map.LoadCache(file);
    fclose(file);
  }

  return success;
}

inline bool
RasterTerrain::SaveCache(FileCache &cache, Path path) const
{
  bool success = false;

  FILE *file = cache.Save(terrain_cache_name, path);
  if (file != nullptr) {
    success = map.SaveCache(file);
    if (success)
      cache.Commit(terrain_cache_name, file);
    else
      cache.Cancel(terrain_cache_name, file);
  }

  return success;
}

inline bool
RasterTerrain::Load(Path path, FileCache *cache,
                    OperationEnvironment &operation)
{
  if (LoadCache(cache, path))
    return true;

  if (!LoadTerrainOverview(dir, map.GetTileCache(), operation))
    return false;

  map.UpdateProjection();

  if (cache != nullptr)
    SaveCache(*cache, path);

  return true;
}

RasterTerrain *
RasterTerrain::OpenTerrain(FileCache *cache, OperationEnvironment &operation)
{
  const auto path = Profile::GetPath(ProfileKeys::MapFile);
  if (path.IsNull())
    return nullptr;

  ZZIP_DIR *dir = zzip_dir_open(NarrowPathName(path), nullptr);
  if (dir == nullptr)
    return nullptr;

  RasterTerrain *rt = new RasterTerrain(dir);
  if (!rt->Load(path, cache, operation)) {
    delete rt;
    return nullptr;
  }

  return rt;
}

bool
RasterTerrain::UpdateTiles(const GeoPoint &location, fixed radius)
{
  auto &tile_cache = map.GetTileCache();
  if (!tile_cache.IsValid())
    return false;

  UpdateTerrainTiles(dir, tile_cache, mutex,
                     map.GetProjection(), location, radius);
  return map.IsDirty();
}
