// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "Asset.hpp"
#ifndef UNICODE
#include "util/UTF8.hpp"
#endif

#include <winuser.h>

#include <string_view>

static std::string_view
EnsureValidUTF8(std::string_view text, char *buffer,
                std::size_t buffer_size) noexcept
{
#ifndef UNICODE
  if (ValidateUTF8(text))
    return text;
  const std::size_t n = SanitizeUTF8(text, {buffer, buffer_size - 1});
  if (n == 0)
    return text;
  return {buffer, n};
#else
  (void)buffer;
  (void)buffer_size;
  return text;
#endif
}

unsigned
TextRenderer::GetHeight(Canvas &canvas, PixelRect rc,
                        std::string_view text) const noexcept
{
  char sanitized[1024];
  const std::string_view safe =
    EnsureValidUTF8(text, sanitized, sizeof(sanitized));
  return canvas.DrawFormattedText(rc, safe, DT_CALCRECT);
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

  char sanitized[1024];
  const std::string_view safe =
    EnsureValidUTF8(text, sanitized, sizeof(sanitized));
  canvas.DrawFormattedText(rc, safe, format);
}
