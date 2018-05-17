/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_TERRAIN_LOADER_HPP
#define XCSOAR_TERRAIN_LOADER_HPP

#include "Thread/SharedMutex.hpp"

struct zzip_dir;
struct GeoPoint;
class RasterTileCache;
class RasterProjection;
class OperationEnvironment;

class TerrainLoader {
  SharedMutex &mutex;

  RasterTileCache &raster_tile_cache;

  const bool scan_overview, scan_tiles;

  OperationEnvironment &env;

  /**
   * The number of remaining segments after the current one.
   */
  mutable unsigned remaining_segments = 0;

public:
  TerrainLoader(SharedMutex &_mutex, RasterTileCache &_rtc,
                bool _scan_overview, bool _scan_all,
                OperationEnvironment &_env)
    :mutex(_mutex), raster_tile_cache(_rtc),
     scan_overview(_scan_overview),
     scan_tiles(!_scan_overview || _scan_all),
     env(_env) {}

  bool LoadOverview(struct zzip_dir *dir,
                    const char *path, const char *world_file);
  bool UpdateTiles(struct zzip_dir *dir, const char *path,
                   int x, int y, unsigned radius);

  /* callback methods for libjasper (via jas_rtc.cpp) */

  long SkipMarkerSegment(long file_offset) const;
  void MarkerSegment(long file_offset, unsigned id);

  void ProcessComment(const char *data, unsigned size);

  void StartTile(unsigned index);

  void SetSize(unsigned width, unsigned height,
               unsigned tile_width, unsigned tile_height,
               unsigned tile_columns, unsigned tile_rows);

  void PutTileData(unsigned index,
                   unsigned start_x, unsigned start_y,
                   unsigned end_x, unsigned end_y,
                   const struct jas_matrix &m);

private:
  bool LoadJPG2000(struct zzip_dir *dir, const char *path);
  void ParseBounds(const char *data);
};

/**
 * @param all load not only overview, but all tiles?  On large files,
 * this is a very expensive operation.  This option was designed for
 * small RASP files only.
 */
bool
LoadTerrainOverview(struct zzip_dir *dir,
                    const char *path, const char *world_file,
                    RasterTileCache &raster_tile_cache,
                    bool all,
                    OperationEnvironment &env);

static inline bool
LoadTerrainOverview(struct zzip_dir *dir,
                    RasterTileCache &tile_cache,
                    OperationEnvironment &env)
{
  return LoadTerrainOverview(dir, "terrain.jp2", "terrain.j2w",
                             tile_cache, false, env);
}

bool
UpdateTerrainTiles(struct zzip_dir *dir, const char *path,
                   RasterTileCache &raster_tile_cache, SharedMutex &mutex,
                   int x, int y, unsigned radius);

static inline bool
UpdateTerrainTiles(struct zzip_dir *dir,
                   RasterTileCache &tile_cache, SharedMutex &mutex,
                   int x, int y, unsigned radius)
{
  return UpdateTerrainTiles(dir, "terrain.jp2", tile_cache, mutex,
                            x, y, radius);
}

bool
UpdateTerrainTiles(struct zzip_dir *dir, const char *path,
                   RasterTileCache &raster_tile_cache, SharedMutex &mutex,
                   const RasterProjection &projection,
                   const GeoPoint &location, double radius);

static inline bool
UpdateTerrainTiles(struct zzip_dir *dir,
                   RasterTileCache &tile_cache, SharedMutex &mutex,
                   const RasterProjection &projection,
                   const GeoPoint &location, double radius)
{
  return UpdateTerrainTiles(dir, "terrain.jp2", tile_cache, mutex,
                            projection, location, radius);
}

#endif
