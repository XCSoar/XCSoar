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

#ifndef XCSOAR_TERRAIN_HEIGHT_MATRIX_HPP
#define XCSOAR_TERRAIN_HEIGHT_MATRIX_HPP

#include "Height.hpp"
#include "Util/AllocatedArray.hxx"

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
    return data.begin();
  }

  const TerrainHeight *GetRow(unsigned y) const {
    return data.begin() + y * width;
  }

  const TerrainHeight *GetDataEnd() const {
    return GetRow(height);
  }
};

#endif
