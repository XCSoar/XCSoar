// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Units/Units.hpp"

struct PixelPoint;
struct PixelSize;
class Canvas;
class Font;
class Pen;

namespace UnitSymbolRenderer
{
  [[gnu::pure]]
  PixelSize GetSize(const Font &font, const Unit unit) noexcept;

  [[gnu::pure]]
  PixelSize GetSize(const Canvas &canvas, const Unit unit) noexcept;

  [[gnu::pure]]
  unsigned GetAscentHeight(const Font &font, const Unit unit) noexcept;

  void Draw(Canvas &canvas, PixelPoint pos, Unit unit,
            const Pen &unit_fraction_pen) noexcept;
}
