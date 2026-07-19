// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BitmapButtonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Bitmap.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

unsigned
BitmapButtonRenderer::GetMinimumButtonWidth() const noexcept
{
  return bitmap.GetSize().width;
}

void
BitmapButtonRenderer::DrawButton(Canvas &canvas, [[maybe_unused]] const PixelRect &rc,
                                 ButtonState state) const noexcept
{
  if (use_alpha) {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
    if (state == ButtonState::PRESSED)
      canvas.StretchNot(bitmap);
    else
      canvas.Stretch(bitmap);
    return;
#elif defined(USE_MEMORY_CANVAS) && !defined(GREYSCALE)
    if (state == ButtonState::PRESSED)
      canvas.StretchNot(bitmap);
    else
      canvas.StretchWithSourceAlpha({0, 0}, canvas.GetSize(), bitmap);
    return;
#endif
  }

  if (state == ButtonState::PRESSED)
    canvas.StretchNot(bitmap);
  else
    canvas.Stretch(bitmap);
}
