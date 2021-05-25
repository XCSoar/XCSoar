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

#ifndef XCSOAR_RASTER_BUFFER_HPP
#define XCSOAR_RASTER_BUFFER_HPP

#include "RasterTraits.hpp"
#include "RasterLocation.hpp"
#include "Height.hpp"
#include "util/AllocatedGrid.hxx"
#include "util/Compiler.h"

class RasterBuffer {
  AllocatedGrid<TerrainHeight> data;

public:
  RasterBuffer() noexcept = default;
  RasterBuffer(unsigned _width, unsigned _height) noexcept
    :data(_width, _height) {}

  RasterBuffer(const RasterBuffer &) = delete;
  RasterBuffer &operator=(const RasterBuffer &) = delete;

  bool IsDefined() const noexcept {
    return data.IsDefined();
  }

  RasterLocation GetSize() const noexcept {
    return {data.GetWidth(), data.GetHeight()};
  }

  RasterLocation GetFineSize() const noexcept {
    return GetSize() << RasterTraits::SUBPIXEL_BITS;
  }

  TerrainHeight *GetData() noexcept {
    return data.begin();
  }

  const TerrainHeight *GetData() const noexcept {
    return data.begin();
  }

  const TerrainHeight *GetDataAt(RasterLocation p) const noexcept {
    return data.GetPointerAt(p.x, p.y);
  }

  void Reset() noexcept {
    data.Reset();
  }

  void Resize(RasterLocation _size) noexcept;

  gcc_pure
  TerrainHeight GetInterpolated(unsigned lx, unsigned ly,
                                unsigned ix, unsigned iy) const noexcept;

  gcc_pure
  TerrainHeight GetInterpolated(RasterLocation p) const noexcept;

  gcc_pure
  TerrainHeight Get(RasterLocation p) const noexcept {
    return *GetDataAt(p);
  }

protected:
  /**
   * Special optimized case for ScanLine(), for NorthUp rendering.
   */
  void ScanHorizontalLine(unsigned ax, unsigned bx, unsigned y,
                          TerrainHeight *buffer, unsigned size,
                          bool interpolate) const noexcept;


public:
  void ScanLine(RasterLocation a, RasterLocation b,
                TerrainHeight *buffer, unsigned size, bool interpolate) const noexcept;

  /**
   * Wrapper for ScanLine() with basic range checks.
   */
  void ScanLineChecked(RasterLocation a, RasterLocation b,
                       TerrainHeight *buffer, unsigned size,
                       bool interpolate) const noexcept;

  gcc_pure
  TerrainHeight GetMaximum() const noexcept;
};

#endif
