/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Util/NonCopyable.hpp"
#include "Util/AllocatedGrid.hpp"
#include "Compiler.h"

#include <cstddef>

class RasterBuffer : private NonCopyable {
public:
  /** invalid value for terrain */
  static constexpr short TERRAIN_INVALID = -32768;
  static constexpr short TERRAIN_WATER_THRESHOLD = -30000;

  enum class TerrainType: uint8_t {
    UNKNOWN, GROUND, WATER
  };

  constexpr
  static bool IsInvalid(short h) {
    return h == TERRAIN_INVALID;
  }

  constexpr
  static bool IsWater(short h) {
    return h <= TERRAIN_WATER_THRESHOLD && !IsInvalid(h);
  }

  constexpr
  static bool IsSpecial(short h) {
    return h <= TERRAIN_WATER_THRESHOLD;
  }

  gcc_const
  static TerrainType GetTerrainType(short h) {
    if (!RasterBuffer::IsSpecial(h))
      return TerrainType::GROUND;

    if (RasterBuffer::IsWater(h))
      return TerrainType::WATER;

    return TerrainType::UNKNOWN;
  }

private:
  AllocatedGrid<short> data;

public:
  RasterBuffer() = default;
  RasterBuffer(unsigned _width, unsigned _height)
    :data(_width, _height) {}

  bool IsDefined() const {
    return data.IsDefined();
  }

  unsigned GetWidth() const {
    return data.GetWidth();
  }

  unsigned GetHeight() const {
    return data.GetHeight();
  }

  unsigned GetFineWidth() const {
    return GetWidth() << 8;
  }

  unsigned GetFineHeight() const {
    return GetHeight() << 8;
  }

  short *GetData() {
    return data.begin();
  }

  const short *GetData() const {
    return data.begin();
  }

  const short *GetDataAt(unsigned x, unsigned y) const {
    return data.GetPointerAt(x, y);
  }

  void Reset() {
    data.Reset();
  }

  void Resize(unsigned _width, unsigned _height);

  gcc_pure
  short GetInterpolated(unsigned lx, unsigned ly,
                        unsigned ix, unsigned iy) const;

  gcc_pure
  short GetInterpolated(unsigned lx, unsigned ly) const;

  gcc_pure
  short Get(unsigned x, unsigned y) const {
    return *GetDataAt(x, y);
  }

protected:
  /**
   * Special optimized case for ScanLine(), for NorthUp rendering.
   */
  void ScanHorizontalLine(unsigned ax, unsigned bx, unsigned y,
                          short *buffer, unsigned size,
                          bool interpolate) const;


public:
  void ScanLine(unsigned ax, unsigned ay, unsigned bx, unsigned by,
                short *buffer, unsigned size, bool interpolate) const;

  /**
   * Wrapper for ScanLine() with basic range checks.
   */
  void ScanLineChecked(unsigned ax, unsigned ay, unsigned bx, unsigned by,
                       short *buffer, unsigned size, bool interpolate) const;

  gcc_pure
  short GetMaximum() const;
};

#endif
