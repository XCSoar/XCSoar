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

#ifndef XCSOAR_TERRAIN_RASTER_MAP_HPP
#define XCSOAR_TERRAIN_RASTER_MAP_HPP

#include "RasterProjection.hpp"
#include "RasterTileCache.hpp"
#include "Geo/GeoPoint.hpp"

class OperationEnvironment;

class RasterMap {
  RasterTileCache raster_tile_cache;
  RasterProjection projection;

public:
  RasterMap() = default;

  RasterMap(const RasterMap &) = delete;
  RasterMap &operator=(const RasterMap &) = delete;

  RasterTileCache &GetTileCache() noexcept {
    return raster_tile_cache;
  }

  void UpdateProjection() noexcept;

  /**
   * Throws on error.
   */
  void SaveCache(BufferedOutputStream &os) const {
    raster_tile_cache.SaveCache(os);
  }

  /**
   * Throws on error.
   */
  void LoadCache(BufferedReader &r);

  bool IsDefined() const noexcept {
    return raster_tile_cache.IsValid();
  }

  const GeoBounds &GetBounds() const noexcept {
    return raster_tile_cache.GetBounds();
  }

  [[gnu::pure]]
  bool IsInside(const GeoPoint &pt) const noexcept {
    return GetBounds().IsInside(pt);
  }

  [[gnu::pure]]
  GeoPoint GetMapCenter() const noexcept {
    return GetBounds().GetCenter();
  }

  /**
   * Determines if UpdateTerrainTiles() should be called again to
   * continue loading.
   */
  [[gnu::pure]]
  bool IsDirty() const noexcept {
    return raster_tile_cache.IsDirty();
  }

  const Serial &GetSerial() const noexcept {
    return raster_tile_cache.GetSerial();
  }

  const RasterProjection &GetProjection() const noexcept {
    return projection;
  }

  /**
   * The geographical distance in meters of the given amount
   * of pixels multiplied by 256.
   *
   * @see RasterProjection::CoarsePixelDistance()
   */
  [[gnu::pure]] double
  PixelDistance(const GeoPoint &location, unsigned pixels) const noexcept {
    return projection.CoarsePixelDistance(location, pixels);
  }

  /**
   * Determine the non-interpolated height at the specified location.
   */
  [[gnu::pure]]
  TerrainHeight GetHeight(const GeoPoint &location) const noexcept;

  /**
   * Determine the interpolated height at the specified location.
   */
  [[gnu::pure]]
  TerrainHeight GetInterpolatedHeight(const GeoPoint &location) const noexcept;

  /**
   * Scan a straight line and fill the buffer with the specified
   * number of samples along the line.
   */
  void ScanLine(const GeoPoint &start, const GeoPoint &end,
                TerrainHeight *buffer, unsigned size,
                bool interpolate) const noexcept;

  struct Intersection {
    GeoPoint location;
    int height;

    constexpr operator bool() const noexcept {
      return location.IsValid();
    }

    static constexpr Intersection Invalid() noexcept {
      return {GeoPoint::Invalid(), 0};
    }
  };

  [[gnu::pure]]
  Intersection FirstIntersection(const GeoPoint &origin, int h_origin,
                                 const GeoPoint &destination,
                                 int h_destination,
                                 int h_virt, int h_ceiling, int h_safety) const noexcept;

  /**
   * Find location where aircraft hits the ground or height_floor
   * @todo margin
   * If the search goes outside the terrain area, will return the destination location
   *
   * @param origin Start (aircraft) location
   * @param h_origin Height of aircraft (m)
   * @param h_glide Height to be glided (m)
   * @param destination Location of aircraft at MSL
   * @param height_floor: minimum height to search
   *
   * @return location of intersection, or GeoPoint::Invalid() if none
   * was found
   */
  [[gnu::pure]]
  GeoPoint GroundIntersection(const GeoPoint &origin,
                              int h_origin, int h_glide,
                              const GeoPoint &destination,
                              const int height_floor) const noexcept;
};


#endif
