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

#ifndef XCSOAR_RASTERTILE_CACHE_HPP
#define XCSOAR_RASTERTILE_CACHE_HPP

#include "RasterTraits.hpp"
#include "RasterTile.hpp"
#include "RasterLocation.hpp"
#include "Geo/GeoBounds.hpp"
#include "util/StaticArray.hxx"
#include "util/Serial.hpp"

#include <cassert>
#include <cstdint>
#include <optional>

#define RASTER_SLOPE_FACT 12

struct jas_matrix;
struct GridLocation;
class BufferedOutputStream;
class BufferedReader;

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

    MarkerSegmentInfo() noexcept = default;

    constexpr MarkerSegmentInfo(uint32_t _file_offset,
                                int _tile=NO_TILE) noexcept
      :file_offset(_file_offset), tile(_tile), count(0) {}

    constexpr bool IsTileSegment() const noexcept {
      return tile != NO_TILE;
    }
  };

  struct CacheHeader {
    static constexpr unsigned VERSION = 0xb;

    unsigned version;
    UnsignedPoint2D size;
    Point2D<uint_least16_t> tile_size;
    UnsignedPoint2D n_tiles;
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
  Point2D<uint_least16_t> tile_size;

  RasterBuffer overview;
  RasterLocation size;
  RasterLocation overview_size_fine;

  GeoBounds bounds;

  StaticArray<MarkerSegmentInfo, 8192> segments;

  /**
   * An array that is used to sort the requested tiles by distance.
   * This is only used by PollTiles() internally, but is stored in the
   * class because it would be too large for the stack.
   */
  StaticArray<uint16_t, MAX_RTC_TILES> request_tiles;

public:
  RasterTileCache() noexcept {
    Reset();
  }

  RasterTileCache(const RasterTileCache &) = delete;
  RasterTileCache &operator=(const RasterTileCache &) = delete;

  void SetBounds(const GeoBounds &_bounds) noexcept {
    assert(_bounds.IsValid());

    bounds = _bounds;
  }

protected:
  void ScanTileLine(GridLocation start, GridLocation end,
                    TerrainHeight *buffer, unsigned size,
                    bool interpolate) const noexcept;

public:
  /**
   * Determine the non-interpolated height at the specified pixel
   * location.
   *
   * @param x the pixel position within the map; may be out of range
   */
  gcc_pure
  TerrainHeight GetHeight(RasterLocation p) const noexcept;

  /**
   * Determine the interpolated height at the specified sub-pixel
   * location.
   *
   * @param p the sub-pixel position within the map; may be out of range
   */
  gcc_pure
  TerrainHeight GetInterpolatedHeight(RasterLocation p) const noexcept;

  /**
   * Scan a straight line and fill the buffer with the specified
   * number of samples along the line.
   *
   * @param start the sub-pixel start location
   * @param end the sub-pixel end location
   */
  void ScanLine(const RasterLocation start, const RasterLocation end,
                TerrainHeight *buffer, unsigned size,
                bool interpolate) const noexcept;

  struct Intersection {
    RasterLocation location;
    int height;
  };

  [[gnu::pure]]
  std::optional<Intersection> FirstIntersection(SignedRasterLocation origin,
                                                SignedRasterLocation destination,
                                                int h_origin,
                                                int h_dest,
                                                int slope_fact, int h_ceiling,
                                                int h_safety,
                                                bool can_climb) const noexcept;

  /**
   * @return {-1,-1} if no intersection was found
   */
  gcc_pure SignedRasterLocation
  GroundIntersection(SignedRasterLocation origin,
                     SignedRasterLocation destination,
                     int h_origin, const int slope_fact,
                     int height_floor) const noexcept;

private:
  /**
   * Get field (not interpolated) directly, without bringing tiles to front.
   * @param p position/256
   * @param tile_index Remember position of active tile, or -1 for overview
   * @return the terrain altitude and a flag that is true when the
   * value was loaded from a "fine" tile
   */
  gcc_pure
  std::pair<TerrainHeight, bool> GetFieldDirect(RasterLocation p) const noexcept;

public:
  /**
   * Throws on error.
   */
  void SaveCache(BufferedOutputStream &os) const;

  /**
   * Throws on error.
   */
  void LoadCache(BufferedReader &r);

  /**
   * Determines if there are still tiles scheduled to be loaded.  Call
   * this after UpdateTiles() to determine if UpdateTiles() should be
   * called again soon.
   */
  bool IsDirty() const noexcept {
    return dirty;
  }

  bool IsValid() const noexcept {
    return bounds.IsValid();
  }

  const Serial &GetSerial() const noexcept {
    return serial;
  }

  void Reset() noexcept;

  const GeoBounds &GetBounds() const noexcept {
    assert(bounds.IsValid());

    return bounds;
  }

public:
  /* methods called by class TerrainLoader */

  gcc_pure
  const MarkerSegmentInfo *
  FindMarkerSegment(uint32_t file_offset) const noexcept;

  long SkipMarkerSegment(long file_offset) const noexcept;
  void MarkerSegment(long file_offset, unsigned id) noexcept;

  void StartTile(unsigned index) noexcept {
    if (!segments.empty() && !segments.back().IsTileSegment())
      /* link current marker segment with this tile */
      segments.back().tile = index;
  }

  void SetSize(UnsignedPoint2D size,
               Point2D<uint_least16_t> tile_size,
               UnsignedPoint2D n_tiles) noexcept;

  void SetLatLonBounds(double lon_min, double lon_max,
                       double lat_min, double lat_max) noexcept;

  void PutOverviewTile(unsigned index,
                       RasterLocation start, RasterLocation end,
                       const struct jas_matrix &m) noexcept;

  bool PollTiles(SignedRasterLocation p, unsigned radius) noexcept;

  void PutTileData(unsigned index, const struct jas_matrix &m) noexcept;

  void FinishTileUpdate() noexcept;

public:
  TerrainHeight GetMaxElevation() const noexcept {
    return overview.GetMaximum();
  }

  /**
   * Is the given point inside the map?
   */
  bool IsInside(RasterLocation p) const noexcept {
    return p.x < size.x && p.y < size.y;
  }

  const auto &GetSize() const noexcept {
    return size;
  }

  RasterLocation GetFineSize() const noexcept {
    return size << RasterTraits::SUBPIXEL_BITS;
  }

private:
  RasterLocation GetFineTileSize() const noexcept {
    return {
      unsigned(tile_size.x) << RasterTraits::SUBPIXEL_BITS,
      unsigned(tile_size.y) << RasterTraits::SUBPIXEL_BITS,
    };
  }
};

#endif
