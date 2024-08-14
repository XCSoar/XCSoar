// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NextArrowRenderer.hpp"
#include "Look/WindArrowLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Math/Screen.hpp"
#include "Math/Angle.hpp"
#include "util/Macros.hpp"

#include <algorithm>

void
NextArrowRenderer::DrawArrow(Canvas &canvas, const PixelRect &rc,
                             Angle angle)
{
  /*
   * Define arrow geometry for forward pointing arrow.
   * These are the coordinates of the corners, relative to the center (o)
   *
   *                               +   (0,-head_len)
   *                              / \
   *                             /   \
   *                            /     \
   * (-head_width,-head_base)  +-+   +-+  (head_width,-head_base)
   *   (-tail_width,-head_base)  | o |  (tail_width,-head_base)
   *                             |   |
   *                             |   |
   *     (-tail_width,tail_len)  +---+  (tail_width,tail_len)
   *
   * The "tail" of the arrow is slightly shorter than the "head" to avoid
   * the corners of the tail to stick out of the bounding PixelRect.
   */
  static constexpr auto head_len = 50;
  static constexpr auto head_width = 36;
  static constexpr auto head_base = head_len - head_width;
  static constexpr auto tail_width = 16;
  static constexpr auto tail_len = head_len - tail_width / 2;

  // An array of the arrow corner coordinates.
  BulkPixelPoint arrow[] = {
    { 0, -head_len },
    { head_width, -head_base },
    { tail_width, -head_base },
    { tail_width, tail_len },
    { -tail_width, tail_len },
    { -tail_width, -head_base },
    { -head_width, -head_base },
  };

  /*
   * Rotate the arrow, center it in the bounding rectangle, and scale
   * it to fill the rectangle.
   *
   * Note that PolygonRotateShift scales a polygon with coordinates
   * in the range -50 to +50 to fill a square with the size of the 'scale'
   * argument.
   */
  const auto size = std::min(rc.GetWidth(), rc.GetHeight());
  PolygonRotateShift(arrow,
                     rc.GetCenter(), angle,
                     size);

  // Draw the arrow.
  canvas.Select(look.next_arrow_pen);
  canvas.Select(look.next_arrow_brush);
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));
}
