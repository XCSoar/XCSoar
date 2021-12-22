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

#ifndef XCSOAR_SCREEN_ROTATED_POLYGON_RENDERER_HPP
#define XCSOAR_SCREEN_ROTATED_POLYGON_RENDERER_HPP

#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Math/Angle.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/CanvasRotateShift.hpp"
#else
#include "Math/Screen.hpp"
#include <array>
#endif

#include <algorithm>

#include <cassert>
#include <span>

class RotatedPolygonRenderer {
#ifdef ENABLE_OPENGL
  const BulkPixelPoint *points;
  CanvasRotateShift rotate_shift;
#else
  std::array<BulkPixelPoint, 64> points;
#endif

public:
  RotatedPolygonRenderer(std::span<const BulkPixelPoint> src,
                         const PixelPoint pos, const Angle angle,
                         const unsigned scale=100)
#ifdef ENABLE_OPENGL
    :points(src.data()), rotate_shift(pos, angle, Layout::Scale(scale / 100.))
#endif
  {
#ifndef ENABLE_OPENGL
    assert(src.size() <= points.size());

    std::copy(src.begin(), src.end(), points.begin());
    PolygonRotateShift(std::span{points}.first(src.size()),
                       pos, angle, Layout::Scale(scale));
#endif
  }

  void Draw(Canvas &canvas, unsigned start, unsigned n) const {
#ifndef ENABLE_OPENGL
    assert(start + n <= points.size());
#endif

    canvas.DrawPolygon(&points[0] + start, n);
  }
};

#endif
