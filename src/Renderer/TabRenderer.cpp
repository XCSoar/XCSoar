// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TabRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Icon.hpp"
#include "Look/DialogLook.hpp"

void
TabRenderer::Draw(Canvas &canvas, const PixelRect &rc,
                  const DialogLook &look,
                  const TCHAR *caption, const MaskedIcon *icon,
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
    text_renderer.Draw(canvas, rc, caption);
  }
}
