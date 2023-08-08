// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HeightMatrix.hpp"
#include "RasterMap.hpp"

#ifdef ENABLE_OPENGL
#include "Geo/GeoBounds.hpp"
#else
#include "Projection/WindowProjection.hpp"
#endif

#include <cassert>

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
  const UnsignedPoint2D round_up{
    quantisation_pixels - 1,
    quantisation_pixels - 1,
  };

  SetSize((_size + round_up) / quantisation_pixels);
}

#ifdef ENABLE_OPENGL

void
HeightMatrix::Fill(const RasterMap &map, const GeoBounds &bounds,
                   const UnsignedPoint2D _size, bool interpolate) noexcept
{
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
  const auto screen_size = projection.GetScreenSize();

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
