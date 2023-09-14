// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Height.hpp"
#include "Math/Point2D.hpp"
#include "util/AllocatedArray.hxx"

class RasterMap;

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
   * Copy values from the #RasterMap to the buffer, north-up only.
   */
  void Fill(const RasterMap &map, const GeoBounds &bounds,
            UnsignedPoint2D _size, bool interpolate) noexcept;
#else
  /**
   * @param interpolate true enables interpolation of sub-pixel values
   */
  void Fill(const RasterMap &map, const WindowProjection &map_projection,
            unsigned quantisation_pixels, bool interpolate) noexcept;
#endif

  UnsignedPoint2D GetSize() const noexcept {
    return size;
  }

  const TerrainHeight *GetData() const noexcept {
    return data.data();
  }

  const TerrainHeight *GetRow(unsigned y) const noexcept {
    return GetData() + y * size.x;
  }

  const TerrainHeight *GetDataEnd() const noexcept {
    return GetRow(size.y);
  }
};
