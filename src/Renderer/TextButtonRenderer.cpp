// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextButtonRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/ButtonLook.hpp"

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

  text_renderer.Draw(canvas, rc, GetCaption());
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

