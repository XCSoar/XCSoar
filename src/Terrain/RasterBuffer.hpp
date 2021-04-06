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
#include "Height.hpp"
#include "util/AllocatedGrid.hxx"
#include "util/Compiler.h"

#include <cstdint>

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

  unsigned GetWidth() const noexcept {
    return data.GetWidth();
  }

  unsigned GetHeight() const noexcept {
    return data.GetHeight();
  }

  unsigned GetFineWidth() const noexcept {
    return GetWidth() << RasterTraits::SUBPIXEL_BITS;
  }

  unsigned GetFineHeight() const noexcept {
    return GetHeight() << RasterTraits::SUBPIXEL_BITS;
  }

  TerrainHeight *GetData() noexcept {
    return data.begin();
  }

  const TerrainHeight *GetData() const noexcept {
    return data.begin();
  }

  const TerrainHeight *GetDataAt(unsigned x, unsigned y) const noexcept {
    return data.GetPointerAt(x, y);
  }

  void Reset() noexcept {
    data.Reset();
  }

  void Resize(unsigned _width, unsigned _height) noexcept;

  gcc_pure
  TerrainHeight GetInterpolated(unsigned lx, unsigned ly,
                                unsigned ix, unsigned iy) const noexcept;

  gcc_pure
  TerrainHeight GetInterpolated(unsigned lx, unsigned ly) const noexcept;

  gcc_pure
  TerrainHeight Get(unsigned x, unsigned y) const noexcept {
    return *GetDataAt(x, y);
  }

protected:
  /**
   * Special optimized case for ScanLine(), for NorthUp rendering.
   */
  void ScanHorizontalLine(unsigned ax, unsigned bx, unsigned y,
                          TerrainHeight *buffer, unsigned size,
                          bool interpolate) const noexcept;


public:
  void ScanLine(unsigned ax, unsigned ay, unsigned bx, unsigned by,
                TerrainHeight *buffer, unsigned size, bool interpolate) const noexcept;

  /**
   * Wrapper for ScanLine() with basic range checks.
   */
  void ScanLineChecked(unsigned ax, unsigned ay, unsigned bx, unsigned by,
                       TerrainHeight *buffer, unsigned size,
                       bool interpolate) const noexcept;

  gcc_pure
  TerrainHeight GetMaximum() const noexcept;
};

#endif
