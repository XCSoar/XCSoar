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

#ifndef XCSOAR_RASTERTILE_HPP
#define XCSOAR_RASTERTILE_HPP

#include "RasterTraits.hpp"
#include "RasterBuffer.hpp"

#include <stdio.h>

struct jas_matrix;

class RasterTile {
  struct MetaData {
    unsigned int xstart, ystart, xend, yend;
  };

public:
  unsigned xstart = 0, ystart = 0, xend = 0, yend = 0;
  unsigned width = 0, height = 0;

  /**
   * The distance of this tile to the center of the screen.  This
   * attribute is used to determine which tiles should be loaded.
   */
  unsigned distance;

  bool request;

  RasterBuffer buffer;

public:
  RasterTile() = default;

  RasterTile(const RasterTile &) = delete;
  RasterTile &operator=(const RasterTile &) = delete;

  void Set(unsigned _xstart, unsigned _ystart,
           unsigned _xend, unsigned _yend) {
    xstart = _xstart;
    ystart = _ystart;
    xend = _xend;
    yend = _yend;
    width = xend - xstart;
    height = yend - ystart;
  }

  /**
   * Permanently disable this tile after a failure.
   */
  void Clear() {
    width = height = 0;
    request = false;
  }

  bool IsDefined() const {
    return width > 0 && height > 0;
  }

  int GetDistance() const {
    return distance;
  }

  bool IsRequested() const {
    return request;
  }

  void SetRequest() {
    request = true;
  }

  void ClearRequest() {
    request = false;
  }

  bool SaveCache(FILE *file) const;
  bool LoadCache(FILE *file);

  gcc_pure
  unsigned CalcDistanceTo(int x, int y) const;

  bool CheckTileVisibility(int view_x, int view_y, unsigned view_radius);

  void Disable() {
    buffer.Reset();
  }

  bool IsEnabled() const {
    return buffer.IsDefined();
  }
  bool IsDisabled() const {
    return !buffer.IsDefined();
  }

  void CopyFrom(const struct jas_matrix &m);

  /**
   * Determine the non-interpolated height at the specified pixel
   * location.
   *
   * @param x the pixel column within the tile; may be out of range
   * @param y the pixel row within the tile; may be out of range
   */
  gcc_pure
  TerrainHeight GetHeight(unsigned x, unsigned y) const;

  /**
   * Determine the interpolated height at the specified sub-pixel
   * location.
   *
   * @param x the pixel column within the tile; may be out of range
   * @param y the pixel row within the tile; may be out of range
   * @param ix the sub-pixel column for interpolation (0..255)
   * @param iy the sub-pixel row for interpolation (0..255)
   */
  gcc_pure
  TerrainHeight GetInterpolatedHeight(unsigned x, unsigned y,
                                      unsigned ix, unsigned iy) const;

  bool VisibilityChanged(int view_x, int view_y, unsigned view_radius);

  void ScanLine(unsigned ax, unsigned ay, unsigned bx, unsigned by,
                TerrainHeight *dest, unsigned size, bool interpolate) const {
    buffer.ScanLine(ax - (xstart << RasterTraits::SUBPIXEL_BITS),
                    ay - (ystart << RasterTraits::SUBPIXEL_BITS),
                    bx - (xstart << RasterTraits::SUBPIXEL_BITS),
                    by - (ystart << RasterTraits::SUBPIXEL_BITS),
                    dest, size, interpolate);
  }
};

#endif
