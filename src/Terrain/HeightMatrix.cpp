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

#include "HeightMatrix.hpp"
#include "RasterMap.hpp"

#ifdef ENABLE_OPENGL
#include "Geo/GeoBounds.hpp"
#else
#include "Projection/WindowProjection.hpp"
#endif

#include <assert.h>

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
  for (auto p = data.begin(), end = p + width * height;
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
  const unsigned screen_width = projection.GetScreenWidth();
  const unsigned screen_height = projection.GetScreenHeight();

  SetSize((screen_width + quantisation_pixels - 1) / quantisation_pixels,
          (screen_height + quantisation_pixels - 1) / quantisation_pixels);

  auto p = data.begin();
  for (unsigned y = 0; y < screen_height;
       y += quantisation_pixels, p += width) {
    map.ScanLine(projection.ScreenToGeo(0, y),
                 projection.ScreenToGeo(screen_width, y),
                 p, width, interpolate);
  }
}

#endif
