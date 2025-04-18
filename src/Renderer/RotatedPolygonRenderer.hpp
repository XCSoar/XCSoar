// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
