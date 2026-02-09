// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SolidContainerWindow.hpp"
#include "ui/canvas/Canvas.hpp"

/**
 * Draw a banded vertical gradient from @p top_color to
 * @p bottom_color using Canvas primitives only (no GL).
 *
 * Inlined here to avoid a dependency on the Renderer layer which is
 * not linked into the screen library.
 */
static void
DrawBandedGradient(Canvas &canvas, const PixelRect &rc,
                   Color top_color, Color bottom_color) noexcept
{
  const int height = rc.GetHeight();
  if (height <= 0)
    return;

  constexpr unsigned N_BANDS = 64;
  for (unsigned i = 0; i < N_BANDS; i++) {
    const int y0 = rc.top + (int)((unsigned)height * i / N_BANDS);
    const int y1 = rc.top + (int)((unsigned)height * (i + 1) / N_BANDS);
#ifdef GREYSCALE
    const uint8_t l = top_color.GetLuminosity()
      + (int(bottom_color.GetLuminosity()) - int(top_color.GetLuminosity()))
        * (int)i / (int)(N_BANDS - 1);
    canvas.DrawFilledRectangle(PixelRect{PixelPoint{rc.left, y0},
                                         PixelPoint{rc.right, y1}},
                               Color(l));
#else
    const uint8_t r = top_color.Red()
      + (int(bottom_color.Red()) - int(top_color.Red()))
        * (int)i / (int)(N_BANDS - 1);
    const uint8_t g = top_color.Green()
      + (int(bottom_color.Green()) - int(top_color.Green()))
        * (int)i / (int)(N_BANDS - 1);
    const uint8_t b = top_color.Blue()
      + (int(bottom_color.Blue()) - int(top_color.Blue()))
        * (int)i / (int)(N_BANDS - 1);
    canvas.DrawFilledRectangle(PixelRect{PixelPoint{rc.left, y0},
                                         PixelPoint{rc.right, y1}},
                               Color(r, g, b));
#endif
  }
}

void
SolidContainerWindow::OnPaint(Canvas &canvas) noexcept
{
  if (has_gradient)
    DrawBandedGradient(canvas, GetClientRect(),
                       gradient_top_color, background_color);
  else
    canvas.Clear(background_color);

  ContainerWindow::OnPaint(canvas);
}
