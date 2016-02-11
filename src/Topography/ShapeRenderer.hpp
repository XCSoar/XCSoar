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

#ifndef SHAPE_RENDERER_HPP
#define SHAPE_RENDERER_HPP

#include "Screen/Pen.hpp"
#include "Screen/Point.hpp"
#include "Util/NonCopyable.hpp"
#include "Util/AllocatedArray.hxx"
#include "Screen/Canvas.hpp"
#include "Screen/Brush.hpp"

#include <assert.h>

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

    canvas.DrawPolyline(points.begin(), num_points);

    num_points = 0;
  }

  void FinishPolygon(Canvas &canvas) {
    if (mode != SOLID) {
      canvas.SelectNullPen();
      canvas.Select(*brush);
      mode = SOLID;
    }

    canvas.DrawPolygon(points.begin(), num_points);

    num_points = 0;
  }

  void Commit() {
    assert(num_points == 0);
  }
};

#endif
