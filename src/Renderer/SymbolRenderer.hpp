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

  void DrawArrow(Canvas &canvas, PixelRect rc, Direction direction) noexcept;
  void DrawSign(Canvas &canvas, PixelRect rc, bool plus) noexcept;
}
