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
HeightMatrix::SetSize(size_t _size)
{
  assert(_size > 0);

  data.GrowDiscard(_size);
}

void
HeightMatrix::SetSize(unsigned _width, unsigned _height)
{
  width = _width;
  height = _height;

  SetSize(width * height);
}

void
HeightMatrix::SetSize(unsigned width, unsigned height,
                      unsigned quantisation_pixels)
{
  SetSize((width + quantisation_pixels - 1) / quantisation_pixels,
          (height + quantisation_pixels - 1) / quantisation_pixels);
}

#ifdef ENABLE_OPENGL

void
HeightMatrix::Fill(const RasterMap &map, const GeoBounds &bounds,
                   unsigned width, unsigned height, bool interpolate)
{
  SetSize(width, height);

  const Angle delta_y = bounds.GetHeight() / height;
  Angle latitude = bounds.GetNorth();
  for (auto p = data.data(), end = p + width * height;
       p != end; p += width, latitude -= delta_y) {
    map.ScanLine(GeoPoint(bounds.GetWest(), latitude),
                 GeoPoint(bounds.GetEast(), latitude),
                 p, width, interpolate);
  }
}

#else

void
HeightMatrix::Fill(const RasterMap &map, const WindowProjection &projection,
                   unsigned quantisation_pixels, bool interpolate)
{
  const auto screen_size = projection.GetScreenSize();

  SetSize((screen_size.width + quantisation_pixels - 1) / quantisation_pixels,
          (screen_size.height + quantisation_pixels - 1) / quantisation_pixels);

  auto p = data.data();
  for (unsigned y = 0; y < screen_size.height;
       y += quantisation_pixels, p += width) {
    map.ScanLine(projection.ScreenToGeo({0, (int)y}),
                 projection.ScreenToGeo({(int)screen_size.width, (int)y}),
                 p, width, interpolate);
  }
}

#endif
