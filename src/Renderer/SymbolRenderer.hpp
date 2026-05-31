// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
class Canvas;

namespace SymbolRenderer
{
  enum Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
  };

  void DrawArrow(Canvas &canvas, PixelRect rc, Direction direction,
                 unsigned max_draw_size=0) noexcept;
  void DrawSign(Canvas &canvas, PixelRect rc, bool plus,
                unsigned max_draw_size=0) noexcept;
  void DrawHamburger(Canvas &canvas, PixelRect rc) noexcept;
}
