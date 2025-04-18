// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  [[gnu::pure]]
  TerrainHeight GetInterpolated(unsigned lx, unsigned ly,
                                unsigned ix, unsigned iy) const noexcept;

  [[gnu::pure]]
  TerrainHeight GetInterpolated(RasterLocation p) const noexcept;

  [[gnu::pure]]
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

  [[gnu::pure]]
  TerrainHeight GetMaximum() const noexcept;
};
