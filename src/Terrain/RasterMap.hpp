/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_TERRAIN_RASTER_MAP_HPP
#define XCSOAR_TERRAIN_RASTER_MAP_HPP

#include "RasterProjection.hpp"
#include "RasterTileCache.hpp"
#include "Geo/GeoPoint.hpp"
#include "Util/NonCopyable.hpp"
#include "Compiler.h"

#include <tchar.h>

class FileCache;
class OperationEnvironment;

class RasterMap : private NonCopyable {
  char *path;
  RasterTileCache raster_tile_cache;
  RasterProjection projection;

public:
  RasterMap(const TCHAR *path, const TCHAR *world_file, FileCache *cache,
            OperationEnvironment &operation);
  ~RasterMap();

  bool IsDefined() const {
    return raster_tile_cache.GetInitialised();
  }

  const GeoBounds &GetBounds() const {
    return raster_tile_cache.GetBounds();
  }

  gcc_pure
  bool IsInside(const GeoPoint &pt) const {
    return GetBounds().IsInside(pt);
  }

  gcc_pure
  GeoPoint GetMapCenter() const {
    return GetBounds().GetCenter();
  }

  void SetViewCenter(const GeoPoint &location, fixed radius);

  /**
   * Determines if SetViewCenter() should be called again to continue
   * loading.
   */
  gcc_pure
  bool IsDirty() const {
    return raster_tile_cache.IsDirty();
  }

  const Serial &GetSerial() const {
    return raster_tile_cache.GetSerial();
  }

  /**
   * @see RasterProjection::CoarsePixelDistance()
   */
  gcc_pure fixed
  PixelDistance(const GeoPoint &location, unsigned pixels) const {
    return projection.CoarsePixelDistance(location, pixels);
  }

  /**
   * Determine the non-interpolated height at the specified location.
   */
  gcc_pure
  short GetHeight(const GeoPoint &location) const;

  /**
   * Determine the interpolated height at the specified location.
   */
  gcc_pure
  short GetInterpolatedHeight(const GeoPoint &location) const;

  /**
   * Scan a straight line and fill the buffer with the specified
   * number of samples along the line.
   */
  void ScanLine(const GeoPoint &start, const GeoPoint &end,
                short *buffer, unsigned size, bool interpolate) const;

  gcc_pure
  bool FirstIntersection(const GeoPoint &origin, int h_origin,
                         const GeoPoint &destination, int h_destination,
                         int h_virt, int h_ceiling, int h_safety,
                         GeoPoint& intx, int &h) const;

  /**
   * Find location where aircraft hits the ground
   * @todo margin
   * If the search goes outside the terrain area, will return the destination location
   *
   * @param origin Start (aircraft) location
   * @param h_origin Height of aircraft (m)
   * @param h_glide Height to be glided (m)
   * @param destination Location of aircraft at MSL
   *
   * @return Location of intersection, or if none, destination
   */
  gcc_pure
  GeoPoint Intersection(const GeoPoint& origin,
                        int h_origin, int h_glide,
                        const GeoPoint& destination) const;

};


#endif
