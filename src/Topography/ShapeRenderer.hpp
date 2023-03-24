// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/dim/Point.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "util/NonCopyable.hpp"
#include "util/AllocatedArray.hxx"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Brush.hpp"

#include <cassert>

/**
 * A helper class optimized for doing bulk draws on OpenGL.
 */
class ShapeRenderer : private NonCopyable {
  AllocatedArray<BulkPixelPoint> points;
  unsigned num_points;

  const Pen *pen;
  const Brush *brush;

  enum { NONE, OUTLINE, SOLID } mode;

public:
  void Configure(const Pen *_pen, const Brush *_brush) {
    pen = _pen;
    brush = _brush;
    mode = NONE;

    num_points = 0;
  }

  void Begin(unsigned n) {
    assert(num_points == 0);

    points.GrowDiscard(((n - 1) | 0x3ff) + 1);
  }

  void AddPoint(PixelPoint pt) {
    assert(num_points < points.size());

    points[num_points++] = pt;
  }

  /**
   * Adds the point only if it a few pixels distant from the previous
   * one.  Useful to reduce the complexity of small figures.
   */
   void AddPointIfDistant(PixelPoint pt) {
    assert(num_points < points.size());

    if (num_points == 0 || ManhattanDistance((PixelPoint)points[num_points - 1], pt) >= 8)
      AddPoint(pt);
  }

  void FinishPolyline(Canvas &canvas) {
    if (mode != OUTLINE) {
      canvas.Select(*pen);
      mode = OUTLINE;
    }

    canvas.DrawPolyline(points.data(), num_points);

    num_points = 0;
  }

  void FinishPolygon(Canvas &canvas) {
    if (mode != SOLID) {
      canvas.SelectNullPen();
      canvas.Select(*brush);
      mode = SOLID;
    }

    canvas.DrawPolygon(points.data(), num_points);

    num_points = 0;
  }

  void Commit() {
    assert(num_points == 0);
  }
};
