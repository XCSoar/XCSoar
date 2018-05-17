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

#ifndef XCSOAR_TERRAIN_RASTER_TERRAIN_HPP
#define XCSOAR_TERRAIN_RASTER_TERRAIN_HPP

#include "RasterMap.hpp"
#include "Geo/GeoPoint.hpp"
#include "Thread/Guard.hpp"
#include "OS/Path.hpp"
#include "IO/ZipArchive.hpp"
#include "Compiler.h"

class FileCache;
class OperationEnvironment;

/**
 * Class to manage raster terrain database, potentially with caching
 * or demand-loading.
 */
class RasterTerrain : public Guard<RasterMap> {
public:
  friend class RoutePlannerGlue; // for route planning
  friend class ProtectedTaskManager; // for intersection
  friend class WaypointVisitorMap; // for intersection rendering

private:
  ZipArchive archive;

  RasterMap map;

private:
  /**
   * Constructor.  Returns uninitialised object.
   */
  explicit RasterTerrain(ZipArchive &&_archive)
    :Guard<RasterMap>(map), archive(std::move(_archive)) {}

public:
  const Serial &GetSerial() const {
    return map.GetSerial();
  }

  /**
   * Load the terrain.  Determines the file to load from profile settings.
   */
  static RasterTerrain *OpenTerrain(FileCache *cache,
                                    OperationEnvironment &operation);

  gcc_pure
  TerrainHeight GetTerrainHeight(const GeoPoint location) const {
    Lease lease(*this);
    return lease->GetHeight(location);
  }

  GeoPoint GetTerrainCenter() const {
    return map.GetMapCenter();
  }

  /**
   * @return true if the method shall be called again
   */
  bool UpdateTiles(const GeoPoint &location, double radius);

private:
  bool LoadCache(FileCache &cache, Path path);

  bool LoadCache(FileCache *cache, Path path) {
    return cache != nullptr && LoadCache(*cache, path);
  }

  bool SaveCache(FileCache &cache, Path path) const;

  bool Load(Path path, FileCache *cache,
            OperationEnvironment &operation);
};

#endif
