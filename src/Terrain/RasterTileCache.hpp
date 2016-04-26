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

#ifndef XCSOAR_RASTERTILE_CACHE_HPP
#define XCSOAR_RASTERTILE_CACHE_HPP

#include "RasterTraits.hpp"
#include "RasterTile.hpp"
#include "RasterLocation.hpp"
#include "Geo/GeoBounds.hpp"
#include "Util/StaticArray.hxx"
#include "Util/Serial.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#define RASTER_SLOPE_FACT 12

struct jas_matrix;
struct GridLocation;

class RasterTileCache {
  static constexpr unsigned MAX_RTC_TILES = 4096;

  /**
   * The maximum number of tiles which are loaded at a time.  This
   * must be limited because the amount of memory is finite.
   */
#if defined(ANDROID)
  static constexpr unsigned MAX_ACTIVE_TILES = 128;
#else
  // desktop: use a lot of memory
  static constexpr unsigned MAX_ACTIVE_TILES = 512;
#endif

  /**
   * The width and height of the terrain bitmap is shifted by this
   * number of bits to determine the overview size.
   */
  static constexpr unsigned OVERVIEW_BITS = 4;

  static constexpr unsigned OVERVIEW_MASK = (~0u) << OVERVIEW_BITS;

  /**
   * Target number of steps in intersection searches; total distance
   * is shifted by this number of bits
   */
  static constexpr unsigned INTERSECT_BITS = 7;

protected:
  friend struct RTDistanceSort;
  friend class TerrainLoader;

  struct MarkerSegmentInfo {
    static constexpr uint16_t NO_TILE = (uint16_t)-1;

    /**
     * The position of this marker segment within the file.
     */
    uint32_t file_offset;

    /**
     * The associated tile number.  -1 if this segment does not belong
     * to a tile.
     */
    uint16_t tile;

    /**
     * The number of follow-up segments.
     */
    uint16_t count;

    MarkerSegmentInfo() {}
    MarkerSegmentInfo(uint32_t _file_offset, int _tile=NO_TILE)
      :file_offset(_file_offset), tile(_tile), count(0) {}

    bool IsTileSegment() const {
      return tile != NO_TILE;
    }
  };

  struct CacheHeader {
    static constexpr unsigned VERSION = 0xb;

    unsigned version;
    unsigned width, height;
    unsigned short tile_width, tile_height;
    unsigned tile_columns, tile_rows;
    unsigned num_marker_segments;
    GeoBounds bounds;
  };

  bool dirty;

  /**
   * This serial gets updated each time the tiles get loaded or
   * discarded.
   */
  Serial serial;

  AllocatedGrid<RasterTile> tiles;
  unsigned short tile_width, tile_height;

  RasterBuffer overview;
  unsigned int width, height;
  unsigned int overview_width_fine, overview_height_fine;

  GeoBounds bounds;

  StaticArray<MarkerSegmentInfo, 8192> segments;

  /**
   * An array that is used to sort the requested tiles by distance.
   * This is only used by PollTiles() internally, but is stored in the
   * class because it would be too large for the stack.
   */
  StaticArray<uint16_t, MAX_RTC_TILES> request_tiles;

public:
  RasterTileCache() {
    Reset();
  }

  RasterTileCache(const RasterTileCache &) = delete;
  RasterTileCache &operator=(const RasterTileCache &) = delete;

  void SetBounds(const GeoBounds &_bounds) {
    assert(_bounds.IsValid());

    bounds = _bounds;
  }

protected:
  void ScanTileLine(GridLocation start, GridLocation end,
                    TerrainHeight *buffer, unsigned size,
                    bool interpolate) const;

public:
  /**
   * Determine the non-interpolated height at the specified pixel
   * location.
   *
   * @param x the pixel column within the map; may be out of range
   * @param y the pixel row within the map; may be out of range
   */
  gcc_pure
  TerrainHeight GetHeight(unsigned x, unsigned y) const;

  /**
   * Determine the interpolated height at the specified sub-pixel
   * location.
   *
   * @param lx the sub-pixel column within the map; may be out of range
   * @param ly the sub-pixel row within the map; may be out of range
   */
  gcc_pure
  TerrainHeight GetInterpolatedHeight(unsigned lx,
                                      unsigned ly) const;

  /**
   * Scan a straight line and fill the buffer with the specified
   * number of samples along the line.
   *
   * @param start the sub-pixel start location
   * @param end the sub-pixel end location
   */
  void ScanLine(const RasterLocation start, const RasterLocation end,
                TerrainHeight *buffer, unsigned size, bool interpolate) const;

  bool FirstIntersection(SignedRasterLocation origin,
                         SignedRasterLocation destination,
                         int h_origin,
                         int h_dest,
                         const int slope_fact, const int h_ceiling,
                         const int h_safety,
                         RasterLocation &_location, int &h_int,
                         const bool can_climb) const;

  /**
   * @return {-1,-1} if no intersection was found
   */
  gcc_pure SignedRasterLocation
  Intersection(SignedRasterLocation origin, SignedRasterLocation destination,
               int h_origin, const int slope_fact,
               const int height_floor) const;

private:
  /**
   * Get field (not interpolated) directly, without bringing tiles to front.
   * @param px X position/256
   * @param px Y position/256
   * @param tile_index Remember position of active tile, or -1 for overview
   * @return the terrain altitude and a flag that is true when the
   * value was loaded from a "fine" tile
   */
  gcc_pure
  std::pair<TerrainHeight, bool> GetFieldDirect(unsigned px, unsigned py) const;

public:
  bool SaveCache(FILE *file) const;
  bool LoadCache(FILE *file);

  /**
   * Determines if there are still tiles scheduled to be loaded.  Call
   * this after UpdateTiles() to determine if UpdateTiles() should be
   * called again soon.
   */
  bool IsDirty() const {
    return dirty;
  }

  bool IsValid() const {
    return bounds.IsValid();
  }

  const Serial &GetSerial() const {
    return serial;
  }

  void Reset();

  const GeoBounds &GetBounds() const {
    assert(bounds.IsValid());

    return bounds;
  }

public:
  /* methods called by class TerrainLoader */

  gcc_pure
  const MarkerSegmentInfo *
  FindMarkerSegment(uint32_t file_offset) const;

  long SkipMarkerSegment(long file_offset) const;
  void MarkerSegment(long file_offset, unsigned id);

  void StartTile(unsigned index) {
    if (!segments.empty() && !segments.back().IsTileSegment())
      /* link current marker segment with this tile */
      segments.back().tile = index;
  }

  void SetSize(unsigned width, unsigned height,
               unsigned tile_width, unsigned tile_height,
               unsigned tile_columns, unsigned tile_rows);

  void SetLatLonBounds(double lon_min, double lon_max,
                       double lat_min, double lat_max);

  void PutOverviewTile(unsigned index,
                       unsigned start_x, unsigned start_y,
                       unsigned end_x, unsigned end_y,
                       const struct jas_matrix &m);

  bool PollTiles(int x, int y, unsigned radius);

  void PutTileData(unsigned index, const struct jas_matrix &m);

  void FinishTileUpdate();

public:
  TerrainHeight GetMaxElevation() const {
    return overview.GetMaximum();
  }

  /**
   * Is the given point inside the map?
   */
  bool IsInside(RasterLocation p) const {
    return p.x < width && p.y < height;
  }

  unsigned int GetWidth() const { return width; }
  unsigned int GetHeight() const { return height; }

  unsigned GetFineWidth() const {
    return width << RasterTraits::SUBPIXEL_BITS;
  }

  unsigned GetFineHeight() const {
    return height << RasterTraits::SUBPIXEL_BITS;
  }

private:
  unsigned GetFineTileWidth() const {
    return tile_width << RasterTraits::SUBPIXEL_BITS;
  }

  unsigned GetFineTileHeight() const {
    return tile_height << RasterTraits::SUBPIXEL_BITS;
  }
};

#endif
