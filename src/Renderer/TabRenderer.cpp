// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TabRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Icon.hpp"
#include "Look/DialogLook.hpp"

void
TabRenderer::Draw(Canvas &canvas, const PixelRect &rc,
                  const DialogLook &look,
                  const char *caption, const MaskedIcon *icon,
                  bool focused, bool pressed, bool selected) const noexcept
{
  canvas.DrawFilledRectangle(rc,
                             look.list.GetBackgroundColor(selected, focused,
                                                          pressed));

  canvas.Select(*look.button.font);
  canvas.SetTextColor(look.list.GetTextColor(selected, focused, pressed));
  canvas.SetBackgroundTransparent();

  if (icon != nullptr) {
    icon->Draw(canvas, rc, selected);
  } else {
    /* draw single-line text centered in the tab button; avoid
       DrawFormattedText / TextRenderer which word-wrap at spaces,
       causing multi-word labels like "Turn Points" to break across
       lines or disappear entirely on GDI */
    const PixelSize size = canvas.CalcTextSize(caption);
    const int x = (rc.left + rc.right - (int)size.width) / 2;
    const int y = (rc.top + rc.bottom - (int)size.height) / 2;
    canvas.DrawClippedText({x, y}, rc, caption);
  }
}
