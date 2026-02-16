// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "Asset.hpp"

#include <winuser.h>

unsigned
TextRenderer::GetHeight(Canvas &canvas, PixelRect rc,
                        std::string_view text) const noexcept
{
  return canvas.DrawFormattedText(rc, text, DT_CALCRECT);
}

unsigned
TextRenderer::GetHeight(Canvas &canvas, unsigned width,
                        std::string_view text) const noexcept
{
  return GetHeight(canvas, PixelRect(0, 0, width, 0), text);
}

unsigned
TextRenderer::GetHeight(const Font &font, unsigned width,
                        std::string_view text) const noexcept
{
  AnyCanvas canvas;
  canvas.Select(font);
  return GetHeight(canvas, width, text);
}

void
TextRenderer::Draw(Canvas &canvas, PixelRect rc,
                   std::string_view text) const noexcept
{
  unsigned format = (center ? DT_CENTER : DT_LEFT);

#ifdef USE_GDI
  if (vcenter) {
    const unsigned height = GetHeight(canvas, rc, text);
    int top = (rc.top + rc.bottom - height) / 2;
    if (top > rc.top)
      rc.top = top;
  }
#else
  if (vcenter)
    format |= DT_VCENTER;

  if (control && IsDithered())
    /* button texts are underlined on the Kobo */
    format |= DT_UNDERLINE;
#endif

  canvas.DrawFormattedText(rc, text, format);
}
