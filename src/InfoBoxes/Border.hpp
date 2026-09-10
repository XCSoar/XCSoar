// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

enum BorderKind_t {
  bkNone,
  bkTop,
  bkRight,
  bkBottom,
  bkLeft
};

#define BORDERTOP    (1<<bkTop)
#define BORDERRIGHT  (1<<bkRight)
#define BORDERBOTTOM (1<<bkBottom)
#define BORDERLEFT   (1<<bkLeft)

class Canvas;
class Pen;

/**
 * Draw the given borders (a bit set of #BORDERTOP, #BORDERRIGHT,
 * #BORDERBOTTOM, #BORDERLEFT) along the edges of the canvas.
 *
 * Note that #BORDERTOP and #BORDERLEFT are drawn on the first
 * row/column while #BORDERBOTTOM and #BORDERRIGHT are drawn on the
 * last one; two neighbouring InfoBoxes therefore draw the edge they
 * share on different pixels, and only one of them may draw it.
 */
void
DrawBorder(Canvas &canvas, unsigned border_kind, const Pen &pen) noexcept;
