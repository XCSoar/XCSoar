// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Height.hpp"
#include "Math/Point2D.hpp"
#include "util/AllocatedArray.hxx"

class RasterMap;
class TerrainBase;

#ifdef ENABLE_OPENGL
class GeoBounds;
#else
class WindowProjection;
#endif

class HeightMatrix {
  AllocatedArray<TerrainHeight> data;
  UnsignedPoint2D size;

public:
  HeightMatrix() noexcept = default;

  HeightMatrix(const HeightMatrix &) = delete;
  HeightMatrix &operator=(const HeightMatrix &) = delete;

protected:
  void SetSize(std::size_t _size) noexcept;
  void SetSize(UnsignedPoint2D _size) noexcept;
  void SetSize(UnsignedPoint2D _size,
               unsigned quantisation_pixels) noexcept;

public:
#ifdef ENABLE_OPENGL
  /**
   * DEM pyramid step from geographic cell size.  Same formula as
   * FillFromTerrain() so a pan strip matches the rest of the matrix.
   */
  [[gnu::pure]]
  static unsigned TerrainQuantisation(const GeoBounds &bounds,
                                      UnsignedPoint2D size) noexcept;

  /**
   * Copy values from the #RasterMap to the buffer, north-up only.
   */
  void Fill(const RasterMap &map, const GeoBounds &bounds,
            UnsignedPoint2D _size, bool interpolate) noexcept;

  /**
   * Copy values from a TerrainBase to the buffer, north-up only.
   * This version reads height data point by point, suitable for
   * terrain implementations that don't use RasterMap's tile cache.
   */
  void FillFromTerrain(const TerrainBase &terrain, const GeoBounds &bounds,
                       UnsignedPoint2D _size) noexcept;
#else
  /**
   * @param interpolate true enables interpolation of sub-pixel values
   */
  void Fill(const RasterMap &map, const WindowProjection &map_projection,
            unsigned quantisation_pixels, bool interpolate) noexcept;

  void FillFromTerrain(const TerrainBase &terrain,
                       const WindowProjection &projection,
                       unsigned quantisation_pixels) noexcept;
#endif

  /**
   * Make height matrix with a gradient from min_h to max_h, default left-right
   */
  void FillGradient(UnsignedPoint2D _size,
                    int16_t min_h, int16_t max_h,
                    bool vertical = false) noexcept;

  UnsignedPoint2D GetSize() const noexcept {
    return size;
  }

  const TerrainHeight *GetData() const noexcept {
    return data.data();
  }

  const TerrainHeight *GetRow(unsigned y) const noexcept {
    return GetData() + y * size.x;
  }

  TerrainHeight *GetRow(unsigned y) noexcept {
    return data.data() + y * size.x;
  }

  /**
   * Slide the samples by an integer number of columns and rows.
   * Positive @p east discards the west edge.  Positive @p north
   * discards the south edge.  The vacated samples are left stale
   * for the caller to fill.
   */
  void Scroll(int east, int north) noexcept;

  const TerrainHeight *GetDataEnd() const noexcept {
    return GetRow(size.y);
  }
};
