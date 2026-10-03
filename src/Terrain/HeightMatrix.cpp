// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HeightMatrix.hpp"
#include "RasterMap.hpp"

#include <cstdlib>
#include <cstring>

#ifdef ENABLE_OPENGL
#include "Geo/GeoBounds.hpp"
#else
#include "Projection/WindowProjection.hpp"
#endif

#include <cassert>

void
HeightMatrix::FillGradient(UnsignedPoint2D _size,
                           int16_t min_h, int16_t max_h,
                           bool vertical) noexcept
{
  SetSize(_size);

  auto *p = data.data();
  const int range = max_h - min_h;
  const unsigned n = vertical ? _size.y : _size.x;
  const int divisor = n > 1 ? (int)(n - 1) : 1;

  for (unsigned y = 0; y < _size.y; ++y)
    for (unsigned x = 0; x < _size.x; ++x)
      *p++ = TerrainHeight(
        (int16_t)(min_h + range *
                  (int)(vertical ? y : x) / divisor));
}

void
HeightMatrix::SetSize(std::size_t _size) noexcept
{
  assert(_size > 0);

  data.GrowDiscard(_size);
}

void
HeightMatrix::SetSize(UnsignedPoint2D _size) noexcept
{
  size = _size;

  SetSize(size.Area());
}

void
HeightMatrix::SetSize(UnsignedPoint2D _size,
                      unsigned quantisation_pixels) noexcept
{
  if (quantisation_pixels < 1)
    quantisation_pixels = 1;

  const UnsignedPoint2D round_up{
    quantisation_pixels - 1,
    quantisation_pixels - 1,
  };

  SetSize((_size + round_up) / quantisation_pixels);
}

void
HeightMatrix::Scroll(int east, int north) noexcept
{
  assert(size.x > 0 && size.y > 0);
  assert(std::abs(east) < (int)size.x);
  assert(std::abs(north) < (int)size.y);

  const unsigned width = size.x;
  const unsigned height = size.y;
  auto *const base = data.data();
  const auto sample_bytes = sizeof(TerrainHeight);

  if (north > 0) {
    const unsigned n = unsigned(north);
    std::memmove(base + n * width, base,
                 (height - n) * width * sample_bytes);
  } else if (north < 0) {
    const unsigned n = unsigned(-north);
    std::memmove(base, base + n * width,
                 (height - n) * width * sample_bytes);
  }

  if (east == 0)
    return;

  unsigned y0 = 0;
  unsigned y1 = height;
  if (north > 0)
    y0 = unsigned(north);
  else if (north < 0)
    y1 = height - unsigned(-north);

  for (unsigned y = y0; y < y1; ++y) {
    auto *const row = base + y * width;
    if (east > 0) {
      const unsigned n = unsigned(east);
      std::memmove(row, row + n, (width - n) * sample_bytes);
    } else {
      const unsigned n = unsigned(-east);
      std::memmove(row + n, row, (width - n) * sample_bytes);
    }
  }
}

#ifdef ENABLE_OPENGL

void
HeightMatrix::Fill(const RasterMap &map, const GeoBounds &bounds,
                   const UnsignedPoint2D _size, bool interpolate) noexcept
{
  if (_size.x == 0 || _size.y == 0)
    return;

  SetSize(_size);

  const Angle delta_y = bounds.GetHeight() / _size.y;
  Angle latitude = bounds.GetNorth();
  for (auto p = data.data(), end = p + _size.Area();
       p != end; p += _size.x, latitude -= delta_y) {
    map.ScanLine(GeoPoint(bounds.GetWest(), latitude),
                 GeoPoint(bounds.GetEast(), latitude),
                 p, _size.x, interpolate);
  }
}

#else

void
HeightMatrix::Fill(const RasterMap &map, const WindowProjection &projection,
                   unsigned quantisation_pixels, bool interpolate) noexcept
{
  if (quantisation_pixels < 1)
    quantisation_pixels = 1;

  const auto screen_size = projection.GetScreenSize();
  if (screen_size.width == 0 || screen_size.height == 0)
    return;

  SetSize((UnsignedPoint2D)screen_size, quantisation_pixels);

  auto p = data.data();
  for (unsigned y = 0; y < screen_size.height;
       y += quantisation_pixels, p += size.x) {
    map.ScanLine(projection.ScreenToGeo({0, (int)y}),
                 projection.ScreenToGeo({(int)screen_size.width, (int)y}),
                 p, size.x, interpolate);
  }
}

#endif
