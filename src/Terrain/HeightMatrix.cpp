// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "HeightMatrix.hpp"
#include "RasterMap.hpp"
#include "LogFile.hpp"

#include <algorithm>
#include <cstdlib>

#ifdef ENABLE_OPENGL
#include "Geo/GeoBounds.hpp"
#else
#include "Projection/WindowProjection.hpp"
#endif

#include <cassert>

/* XCSOAR_TERRAIN_SLOW=N scans each row N times (capped).  A full
   rebuild then costs N times a strip, which is the mistake the
   benchmark is trying to see. */
static unsigned
TerrainScanRepeats() noexcept
{
  static const unsigned repeats = []() noexcept {
    const char *env = std::getenv("XCSOAR_TERRAIN_SLOW");
    if (env == nullptr || *env == '\0')
      return 1u;
    const int v = std::atoi(env);
    if (v <= 1)
      return 1u;
    const unsigned capped = std::min(unsigned(v), 32u);
    LogFmt("Terrain scan repeated {} times (XCSOAR_TERRAIN_SLOW)",
           capped);
    return capped;
  }();
  return repeats;
}

static void
ScanTerrainLine(const RasterMap &map,
                const GeoPoint a, const GeoPoint b,
                TerrainHeight *row, unsigned width,
                bool interpolate) noexcept
{
  const unsigned n = TerrainScanRepeats();
  for (unsigned i = 0; i < n; ++i)
    map.ScanLine(a, b, row, width, interpolate);
}

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

  SlideGrid(data.data(), size.x, size.x, size.y,
            sizeof(TerrainHeight), east, north);
}

#ifdef ENABLE_OPENGL

void
HeightMatrix::FillRect(const RasterMap &map, const GeoBounds &bounds,
                       unsigned x, unsigned y,
                       unsigned w, unsigned h,
                       bool interpolate) noexcept
{
  if (w < 2 || h == 0 || size.x < 2)
    return;

  const unsigned height = size.y;
  const Angle dlat = bounds.GetHeight() / double(height);
  const Angle width_angle = bounds.GetWidth();
  const double x_denom = double(size.x - 1);
  const Angle west = bounds.GetWest();
  const Angle north = bounds.GetNorth();

  for (unsigned row = y; row < y + h; ++row) {
    const Angle lat = north - dlat * double(row);
    const Angle lon0 = west + width_angle * (double(x) / x_denom);
    const Angle lon1 =
      west + width_angle * (double(x + w - 1) / x_denom);
    ScanTerrainLine(map, GeoPoint(lon0, lat), GeoPoint(lon1, lat),
                    GetRow(row) + x, w, interpolate);
  }
}

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
       p != end; p += _size.x, latitude -= delta_y)
    ScanTerrainLine(map, GeoPoint(bounds.GetWest(), latitude),
                    GeoPoint(bounds.GetEast(), latitude),
                    p, _size.x, interpolate);
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
    ScanTerrainLine(map,
                    projection.ScreenToGeo({0, (int)y}),
                    projection.ScreenToGeo({(int)screen_size.width,
                                            (int)y}),
                    p, size.x, interpolate);
  }
}

#endif
