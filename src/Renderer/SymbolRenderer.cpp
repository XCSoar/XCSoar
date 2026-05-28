// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SymbolRenderer.hpp"
#include "ui/canvas/Canvas.hpp"

#include <algorithm>

namespace {

[[gnu::pure]]
unsigned
MinDimension(const PixelRect &rc) noexcept
{
  return std::min(rc.GetWidth(), rc.GetHeight());
}

/** One fifth of the shorter rc side; used for arrows and bar symbols. */
[[gnu::pure]]
unsigned
DrawSize(const PixelRect &rc, unsigned max_draw_size) noexcept
{
  unsigned size = std::max(1u, MinDimension(rc) / 5);
  if (max_draw_size > 0)
    size = std::min(size, max_draw_size);

  return size;
}

/**
 * Horizontal bar half-extents (WithMargin size).
 * @param min_dim_for_width if non-zero, bar is 75% of this (menu icon)
 */
[[gnu::pure]]
PixelSize
BarMargin(unsigned draw_size, unsigned min_dim_for_width) noexcept
{
  const unsigned width = min_dim_for_width > 0
    ? std::max(1u, min_dim_for_width * 3 / 8)
    : draw_size;
  return {width, std::max(1u, draw_size / 3)};
}

void
DrawBarAt(Canvas &canvas, PixelPoint center, PixelSize margin) noexcept
{
  canvas.DrawRectangle(PixelRect{center}.WithMargin(margin));
}

} // namespace

void
SymbolRenderer::DrawArrow(Canvas &canvas, PixelRect rc,
                          Direction direction,
                          unsigned max_draw_size) noexcept
{
  assert(direction == UP || direction == DOWN ||
         direction == LEFT || direction == RIGHT);

  const unsigned size = DrawSize(rc, max_draw_size);
  const auto center = rc.GetCenter();
  BulkPixelPoint arrow[3];

  if (direction == LEFT || direction == RIGHT) {
    arrow[0].x = center.x + (direction == LEFT ? size : -size);
    arrow[0].y = center.y + size;
    arrow[1].x = center.x + (direction == LEFT ? -size : size);
    arrow[1].y = center.y;
    arrow[2].x = center.x + (direction == LEFT ? size : -size);
    arrow[2].y = center.y - size;
  } else if (direction == UP || direction == DOWN) {
    arrow[0].x = center.x + size;
    arrow[0].y = center.y + (direction == UP ? size : -size);
    arrow[1].x = center.x;
    arrow[1].y = center.y + (direction == UP ? -size : size);
    arrow[2].x = center.x - size;
    arrow[2].y = center.y + (direction == UP ? size : -size);
  }

  canvas.DrawTriangleFan(arrow, 3);
}

void
SymbolRenderer::DrawSign(Canvas &canvas, PixelRect rc, bool plus,
                         unsigned max_draw_size) noexcept
{
  if (MinDimension(rc) == 0)
    return;

  const unsigned draw_size = DrawSize(rc, max_draw_size);
  const auto center = rc.GetCenter();
  const PixelSize margin = BarMargin(draw_size, 0);

  DrawBarAt(canvas, center, margin);

  if (plus)
    DrawBarAt(canvas, center, {margin.height, margin.width});
}

void
SymbolRenderer::DrawHamburger(Canvas &canvas, PixelRect rc) noexcept
{
  const unsigned min_dim = MinDimension(rc);
  if (min_dim == 0)
    return;

  const unsigned draw_size = DrawSize(rc, 0);
  const auto center = rc.GetCenter();
  const PixelSize margin = BarMargin(draw_size, min_dim);
  const int step = int(2 * margin.height + std::max(1u, draw_size / 3));

  DrawBarAt(canvas, {center.x, center.y - step}, margin);
  DrawBarAt(canvas, center, margin);
  DrawBarAt(canvas, {center.x, center.y + step}, margin);
}
