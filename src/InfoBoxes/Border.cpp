// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Border.hpp"
#include "ui/canvas/Canvas.hpp"

void
DrawBorder(Canvas &canvas, unsigned border_kind, const Pen &pen) noexcept
{
  if (border_kind == 0)
    return;

  canvas.Select(pen);

  const int width = canvas.GetWidth(), height = canvas.GetHeight();

  if (border_kind & BORDERTOP)
    canvas.DrawExactLine({0, 0}, {width - 1, 0});

  if (border_kind & BORDERRIGHT)
    canvas.DrawExactLine({width - 1, 0}, {width - 1, height});

  if (border_kind & BORDERBOTTOM)
    canvas.DrawExactLine({0, height - 1}, {width - 1, height - 1});

  if (border_kind & BORDERLEFT)
    canvas.DrawExactLine({0, 0}, {0, height - 1});
}
