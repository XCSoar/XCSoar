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

#ifndef XCSOAR_TERRAIN_LOADER_HPP
#define XCSOAR_TERRAIN_LOADER_HPP

#include "Math/fixed.hpp"

#include <tchar.h>

struct GeoPoint;
class RasterTileCache;
class RasterProjection;
class OperationEnvironment;

class TerrainLoader {
  RasterTileCache &raster_tile_cache;

  const bool scan_overview;

  OperationEnvironment &env;

  /**
   * The number of remaining segments after the current one.
   */
  mutable unsigned remaining_segments;

public:
  TerrainLoader(RasterTileCache &_rtc, bool _scan_overview,
                OperationEnvironment &_env)
    :raster_tile_cache(_rtc), scan_overview(_scan_overview), env(_env), remaining_segments(0) {}

  bool LoadOverview(const TCHAR *path, const TCHAR *world_file);
  bool UpdateTiles(const TCHAR *path, int x, int y, unsigned radius);

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
  bool LoadJPG2000(const TCHAR *path);
  void ParseBounds(const char *data);
};

bool
LoadTerrainOverview(const TCHAR *path, const TCHAR *world_file,
                    RasterTileCache &raster_tile_cache,
                    OperationEnvironment &env);

bool
UpdateTerrainTiles(const TCHAR *path,
                   RasterTileCache &raster_tile_cache,
                   int x, int y, unsigned radius);

bool
UpdateTerrainTiles(const TCHAR *path,
                   RasterTileCache &raster_tile_cache,
                   const RasterProjection &projection,
                   const GeoPoint &location, fixed radius);

#endif
