// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextButtonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/ButtonLook.hpp"
#ifndef UNICODE
#include "util/UTF8.hpp"
#endif

#include <tchar.h>

#ifndef UNICODE
static tstring_view
SafeForTextSize(tstring_view text, char *buffer,
                std::size_t buffer_size) noexcept
{
  if (ValidateUTF8(text))
    return text;
  const std::size_t n = SanitizeUTF8(text, {buffer, buffer_size - 1});
  if (n == 0)
    return text;
  buffer[n] = '\0';
  return buffer;
}
#endif

void
TextButtonRenderer::SetCaption2(StaticString<32>::const_pointer _caption2) noexcept
{
  caption2 = _caption2 != nullptr ? _caption2 : _T("");
  if (caption2.empty())
    caption3.clear();
  text_renderer.InvalidateLayout();
}

void
TextButtonRenderer::SetCaption3(StaticString<32>::const_pointer _caption3) noexcept
{
  caption3 = _caption3 != nullptr ? _caption3 : _T("");
  text_renderer.InvalidateLayout();
}

unsigned
TextButtonRenderer::GetMinimumButtonWidth(const ButtonLook &look,
                                          std::string_view caption) noexcept
{
#ifndef UNICODE
  char sanitized[256];
  const tstring_view safe = SafeForTextSize(caption, sanitized, sizeof(sanitized));
  return 2 * (ButtonFrameRenderer::GetMargin() + Layout::GetTextPadding())
    + look.font->TextSize(safe).width;
#else
  return 2 * (ButtonFrameRenderer::GetMargin() + Layout::GetTextPadding())
    + look.font->TextSize(caption).width;
#endif
}

inline void
TextButtonRenderer::DrawCaption(Canvas &canvas, const PixelRect &rc,
                                ButtonState state) const noexcept
{
  const ButtonLook &look = GetLook();

  canvas.SetBackgroundTransparent();

  switch (state) {
  case ButtonState::DISABLED:
    canvas.SetTextColor(look.disabled.color);
    break;

  case ButtonState::FOCUSED:
  case ButtonState::PRESSED:
    canvas.SetTextColor(look.focused.foreground_color);
    break;

  case ButtonState::SELECTED:
    canvas.SetTextColor(look.selected.foreground_color);
    break;

  case ButtonState::ENABLED:
    canvas.SetTextColor(look.standard.foreground_color);
    break;
  }

  canvas.Select(*look.font);

  const unsigned line_height = look.font->GetLineSpacing();
  if (!caption3.empty()) {
    const unsigned total_height = 3 * line_height;
    const int block_top = (rc.top + rc.bottom - (int)total_height) / 2;
    text_renderer.Draw(canvas,
                       PixelRect(rc.left, block_top, rc.right,
                                 block_top + (int)line_height),
                       GetCaption());
    text_renderer.Draw(canvas,
                       PixelRect(rc.left, block_top + (int)line_height,
                                 rc.right, block_top + 2 * (int)line_height),
                       caption2.c_str());
    text_renderer.Draw(canvas,
                       PixelRect(rc.left, block_top + 2 * (int)line_height,
                                 rc.right, block_top + (int)total_height),
                       caption3.c_str());
  } else if (!caption2.empty()) {
    const unsigned total_height = 2 * line_height;
    const int block_top = (rc.top + rc.bottom - (int)total_height) / 2;
    text_renderer.Draw(canvas,
                       PixelRect(rc.left, block_top, rc.right,
                                 block_top + (int)line_height),
                       GetCaption());
    text_renderer.Draw(canvas,
                       PixelRect(rc.left, block_top + (int)line_height,
                                 rc.right, block_top + (int)total_height),
                       caption2.c_str());
  } else
    text_renderer.Draw(canvas, rc, GetCaption());
}

unsigned
TextButtonRenderer::GetMinimumButtonWidth() const noexcept
{
#ifndef UNICODE
  char sanitized[256];
  const tstring_view safe_caption =
    SafeForTextSize(caption.c_str(), sanitized, sizeof(sanitized));
  unsigned w = GetLook().font->TextSize(safe_caption).width;
  if (!caption2.empty()) {
    const tstring_view safe_caption2 =
      SafeForTextSize(caption2.c_str(), sanitized, sizeof(sanitized));
    const unsigned w2 = GetLook().font->TextSize(safe_caption2).width;
    if (w2 > w)
      w = w2;
  }
  if (!caption3.empty()) {
    const tstring_view safe_caption3 =
      SafeForTextSize(caption3.c_str(), sanitized, sizeof(sanitized));
    const unsigned w3 = GetLook().font->TextSize(safe_caption3).width;
    if (w3 > w)
      w = w3;
  }
#else
  unsigned w = GetLook().font->TextSize(caption.c_str()).width;
  if (!caption2.empty()) {
    const unsigned w2 = GetLook().font->TextSize(caption2.c_str()).width;
    if (w2 > w)
      w = w2;
  }
  if (!caption3.empty()) {
    const unsigned w3 = GetLook().font->TextSize(caption3.c_str()).width;
    if (w3 > w)
      w = w3;
  }
#endif
  return 2 * (frame_renderer.GetMargin() + Layout::GetTextPadding()) + w;
}

void
TextButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                               ButtonState state) const noexcept
{
  frame_renderer.DrawButton(canvas, rc, state);

  if (!caption.empty() || !caption2.empty() || !caption3.empty())
    DrawCaption(canvas, frame_renderer.GetDrawingRect(rc, state),
                state);
}

