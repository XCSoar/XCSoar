// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ColorButtonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"

void
ColorButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                ButtonState state) const noexcept
{
  frame_renderer.DrawButton(canvas, rc, state);

  PixelRect fill_rc = rc;
  fill_rc.Grow(-3);
  canvas.DrawFilledRectangle(fill_rc, color);
}
