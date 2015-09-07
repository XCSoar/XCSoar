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

#include "Terrain/RasterTerrain.hpp"
#include "Profile/Profile.hpp"
#include "IO/FileCache.hpp"

#include <windef.h> /* for MAX_PATH */

#include <string.h>

/* use separate cache files for FIXED=y and FIXED=n because the file
   format is different */
#ifdef FIXED_MATH
static const TCHAR *const terrain_cache_name = _T("terrain_fixed");
#else
static const TCHAR *const terrain_cache_name = _T("terrain");
#endif

inline bool
RasterTerrain::LoadCache(FileCache &cache, const TCHAR *path)
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
RasterTerrain::SaveCache(FileCache &cache, const TCHAR *path) const
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
RasterTerrain::Load(const TCHAR *path, const TCHAR *world_file,
                    FileCache *cache, OperationEnvironment &operation)
{
  if (LoadCache(cache, path))
    return true;

  if (!map.Load(path, world_file, operation))
    return false;

  if (cache != nullptr)
    SaveCache(*cache, path);

  return true;
}

RasterTerrain *
RasterTerrain::OpenTerrain(FileCache *cache, OperationEnvironment &operation)
{
  TCHAR path[MAX_PATH], world_file_buffer[MAX_PATH];
  const TCHAR *world_file;

  if (Profile::GetPath(ProfileKeys::MapFile, path)) {
    _tcscpy(world_file_buffer, path);
    _tcscat(world_file_buffer, _T("/terrain.j2w"));
    world_file = world_file_buffer;

    _tcscat(path, _T("/terrain.jp2"));
  } else
    return nullptr;

  RasterTerrain *rt = new RasterTerrain(path);
  if (!rt->Load(path, world_file, cache, operation)) {
    delete rt;
    return nullptr;
  }

  return rt;
}
