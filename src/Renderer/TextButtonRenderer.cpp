// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextButtonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/ButtonLook.hpp"

#ifdef TARGET_IS_KOBO_NICKEL
#include <algorithm>

static void
PrepareKoboNickelButtonCaption(Canvas &canvas, const PixelRect &rc,
                               ButtonState state, PixelPoint position,
                               PixelSize size) noexcept
{
  const bool inverted = state == ButtonState::FOCUSED ||
    state == ButtonState::PRESSED;
  const PixelRect text_rc{
    position.x, position.y,
    std::min(rc.right, position.x + int(size.width)),
    std::min(rc.bottom, position.y + int(size.height)),
  };

  /* Temporary until FBInk glyph blending is root-caused on the devices. */
  canvas.DrawFilledRectangle(text_rc, inverted ? COLOR_BLACK : COLOR_WHITE);
  canvas.SetTextColor(inverted ? COLOR_WHITE : COLOR_BLACK);
}
#endif

unsigned
TextButtonRenderer::GetMinimumButtonWidth(const ButtonLook &look,
                                          std::string_view caption) noexcept
{
  return 2 * (ButtonFrameRenderer::GetMargin() + Layout::GetTextPadding())
    + look.font->TextSize(caption).width;
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

#ifdef TARGET_IS_KOBO_NICKEL
  const PixelSize text_size = canvas.CalcTextSize(GetCaption());
  const int x = rc.left +
    std::max(0, int(rc.GetWidth()) - int(text_size.width)) / 2;
  const int y = rc.top +
    std::max(0, int(rc.GetHeight()) - int(text_size.height)) / 2;
  const PixelPoint position{x, y};
  PrepareKoboNickelButtonCaption(canvas, rc, state, position, text_size);
  canvas.DrawClippedText(position, rc.right - x, GetCaption());
#else
  text_renderer.Draw(canvas, rc, GetCaption());
#endif
}

unsigned
TextButtonRenderer::GetMinimumButtonWidth() const noexcept
{
  return 2 * (frame_renderer.GetMargin() + Layout::GetTextPadding())
    + GetLook().font->TextSize(caption.c_str()).width;
}

void
TextButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                               ButtonState state) const noexcept
{
  frame_renderer.DrawButton(canvas, rc, state);

  if (!caption.empty())
    DrawCaption(canvas, frame_renderer.GetDrawingRect(rc, state),
                state);
}
