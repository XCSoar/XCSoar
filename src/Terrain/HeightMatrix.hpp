// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Height.hpp"
#include "Math/Point2D.hpp"
#include "util/AllocatedArray.hxx"

#include <cstring>

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

  /**
   * Fill one rectangle of an existing matrix.  Sample @c i of a row
   * lies at @c i/(width-1) of the west–east span, matching ScanLine().
   */
  void FillRect(const RasterMap &map, const GeoBounds &bounds,
                unsigned x, unsigned y, unsigned w, unsigned h,
                bool interpolate) noexcept;
#else
  /**
   * @param interpolate true enables interpolation of sub-pixel values
   */
  void Fill(const RasterMap &map, const WindowProjection &map_projection,
            unsigned quantisation_pixels, bool interpolate) noexcept;
#endif

  /**
   * Make height matrix with a gradient from min_h to max_h, default left-right
   */
  void FillGradient(UnsignedPoint2D _size,
                    int16_t min_h, int16_t max_h,
                    bool vertical = false) noexcept;

  UnsignedPoint2D GetSize() const noexcept {
    return size;
  }

  const TerrainHeight *GetData() const noexcept {
    return data.data();
  }

  const TerrainHeight *GetRow(unsigned y) const noexcept {
    return GetData() + y * size.x;
  }

  TerrainHeight *GetRow(unsigned y) noexcept {
    return data.data() + y * size.x;
  }

  /**
   * Slide the samples by an integer number of columns and rows.
   * Positive @p east discards the west edge.  Positive @p north
   * discards the south edge.  The vacated samples are left stale
   * for the caller to fill.
   */
  void Scroll(int east, int north) noexcept;

  const TerrainHeight *GetDataEnd() const noexcept {
    return GetRow(size.y);
  }
};

/**
 * Slide a row-major grid by whole samples.  @p stride is the row
 * pitch in samples and may be wider than @p width.  Positive @p east
 * discards the west edge.  Positive @p north discards the south edge.
 * The vacated samples are left as they were.
 */
inline void
SlideGrid(void *base, unsigned stride, unsigned width, unsigned height,
          unsigned sample_bytes, int east, int north) noexcept
{
  if ((east == 0 && north == 0) || width == 0 || height == 0 ||
      stride < width)
    return;

  auto *const bytes = static_cast<unsigned char *>(base);
  const unsigned row_bytes = stride * sample_bytes;
  const unsigned used_bytes = width * sample_bytes;
  auto row = [bytes, row_bytes](unsigned y) {
    return bytes + std::size_t(y) * row_bytes;
  };

  /* Packed rows are one block.  A wider pitch has gaps, so each
     row moves on its own, from the end that would be overwritten. */
  if (north > 0) {
    const unsigned n = unsigned(north);
    if (n < height) {
      if (stride == width)
        std::memmove(bytes + std::size_t(n) * row_bytes, bytes,
                     std::size_t(height - n) * row_bytes);
      else
        for (unsigned src = height - n; src > 0; --src)
          std::memmove(row(src + n - 1), row(src - 1), used_bytes);
    }
  } else if (north < 0) {
    const unsigned n = unsigned(-north);
    if (n < height) {
      if (stride == width)
        std::memmove(bytes, bytes + std::size_t(n) * row_bytes,
                     std::size_t(height - n) * row_bytes);
      else
        for (unsigned y = 0; y < height - n; ++y)
          std::memmove(row(y), row(y + n), used_bytes);
    }
  }

  if (east == 0 || unsigned(east > 0 ? east : -east) >= width)
    return;

  unsigned y0 = 0;
  unsigned y1 = height;
  if (north > 0 && unsigned(north) < height)
    y0 = unsigned(north);
  else if (north < 0 && unsigned(-north) < height)
    y1 = height - unsigned(-north);

  const unsigned n = unsigned(east > 0 ? east : -east);
  const unsigned keep = (width - n) * sample_bytes;
  const unsigned shift = n * sample_bytes;
  for (unsigned y = y0; y < y1; ++y) {
    auto *const r = row(y);
    if (east > 0)
      std::memmove(r, r + shift, keep);
    else
      std::memmove(r + shift, r, keep);
  }
}
