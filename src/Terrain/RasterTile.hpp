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

#ifndef XCSOAR_RASTERTILE_HPP
#define XCSOAR_RASTERTILE_HPP

#include "RasterTraits.hpp"
#include "RasterLocation.hpp"
#include "RasterBuffer.hpp"

struct jas_matrix;
class BufferedOutputStream;
class BufferedReader;

class RasterTile {
  struct MetaData {
    RasterLocation start, end;
  };

public:
  RasterLocation start{0, 0}, end, size{0, 0};

  /**
   * The distance of this tile to the center of the screen.  This
   * attribute is used to determine which tiles should be loaded.
   */
  unsigned distance;

  bool request;

  RasterBuffer buffer;

public:
  RasterTile() noexcept = default;

  RasterTile(const RasterTile &) = delete;
  RasterTile &operator=(const RasterTile &) = delete;

  void Set(RasterLocation _start, RasterLocation _end) noexcept {
    start = _start;
    end = _end;
    size = end - start;
  }

  /**
   * Permanently disable this tile after a failure.
   */
  void Clear() noexcept {
    size = {0, 0};
    request = false;
  }

  bool IsDefined() const noexcept {
    return size.x > 0 && size.y > 0;
  }

  int GetDistance() const noexcept {
    return distance;
  }

  bool IsRequested() const noexcept {
    return request;
  }

  void SetRequest() noexcept {
    request = true;
  }

  void ClearRequest() noexcept {
    request = false;
  }

  void SaveCache(BufferedOutputStream &os) const;
  void LoadCache(BufferedReader &r);

  gcc_pure
  unsigned CalcDistanceTo(IntPoint2D p) const noexcept;

  bool CheckTileVisibility(IntPoint2D view, unsigned view_radius) noexcept;

  void Unload() noexcept {
    buffer.Reset();
  }

  bool IsLoaded() const noexcept {
    return buffer.IsDefined();
  }

  void CopyFrom(const struct jas_matrix &m) noexcept;

  /**
   * Determine the non-interpolated height at the specified pixel
   * location.
   *
   * @param x the pixel column within the tile; may be out of range
   * @param y the pixel row within the tile; may be out of range
   */
  gcc_pure
  TerrainHeight GetHeight(RasterLocation p) const noexcept;

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
                                      unsigned ix, unsigned iy) const noexcept;

  bool VisibilityChanged(IntPoint2D view, unsigned view_radius) noexcept;

  void ScanLine(RasterLocation a, RasterLocation b,
                TerrainHeight *dest, unsigned dest_size,
                bool interpolate) const noexcept {
    buffer.ScanLine(a - (start << RasterTraits::SUBPIXEL_BITS),
                    b - (start << RasterTraits::SUBPIXEL_BITS),
                    dest, dest_size, interpolate);
  }
};

#endif
