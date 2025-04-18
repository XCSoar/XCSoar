// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  [[gnu::pure]]
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
  [[gnu::pure]]
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
  [[gnu::pure]]
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
