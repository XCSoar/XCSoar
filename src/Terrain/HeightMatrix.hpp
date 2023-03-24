// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Height.hpp"
#include "util/AllocatedArray.hxx"

class RasterMap;

#ifdef ENABLE_OPENGL
class GeoBounds;
#else
class WindowProjection;
#endif

class HeightMatrix {
  AllocatedArray<TerrainHeight> data;
  unsigned width, height;

public:
  HeightMatrix():width(0), height(0) {}

  HeightMatrix(const HeightMatrix &) = delete;
  HeightMatrix &operator=(const HeightMatrix &) = delete;

protected:
  void SetSize(size_t _size);
  void SetSize(unsigned width, unsigned height);
  void SetSize(unsigned width, unsigned height, unsigned quantisation_pixels);

public:
#ifdef ENABLE_OPENGL
  /**
   * Copy values from the #RasterMap to the buffer, north-up only.
   */
  void Fill(const RasterMap &map, const GeoBounds &bounds,
            unsigned _width, unsigned _height, bool interpolate);
#else
  /**
   * @param interpolate true enables interpolation of sub-pixel values
   */
  void Fill(const RasterMap &map, const WindowProjection &map_projection,
            unsigned quantisation_pixels, bool interpolate);
#endif

  unsigned GetWidth() const {
    return width;
  }

  unsigned GetHeight() const {
    return height;
  }

  const TerrainHeight *GetData() const {
    return data.data();
  }

  const TerrainHeight *GetRow(unsigned y) const {
    return GetData() + y * width;
  }

  const TerrainHeight *GetDataEnd() const {
    return GetRow(height);
  }
};
