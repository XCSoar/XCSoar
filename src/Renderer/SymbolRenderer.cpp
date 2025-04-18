// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SymbolRenderer.hpp"
#include "ui/canvas/Canvas.hpp"

#include <algorithm>

void
SymbolRenderer::DrawArrow(Canvas &canvas, PixelRect rc,
                          Direction direction) noexcept
{
  assert(direction == UP || direction == DOWN ||
         direction == LEFT || direction == RIGHT);

  auto size = std::min(rc.GetWidth(), rc.GetHeight()) / 5;
  auto center = rc.GetCenter();
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
SymbolRenderer::DrawSign(Canvas &canvas, PixelRect rc, bool plus) noexcept
{
  unsigned size = std::min(rc.GetWidth(), rc.GetHeight()) / 5;
  const auto horizontal_rect = PixelRect{rc.GetCenter()}
    .WithMargin({size, size / 3});

  // Draw horizontal bar
  canvas.DrawRectangle(horizontal_rect);

  if (plus) {
    // Draw vertical bar
    const auto vertical_rect = PixelRect{rc.GetCenter()}
      .WithMargin({size / 3, size});
    canvas.DrawRectangle(vertical_rect);
  }
}
