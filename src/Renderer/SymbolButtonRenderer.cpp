// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SymbolButtonRenderer.hpp"
#include "SymbolRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/ButtonLook.hpp"

bool
SymbolButtonRenderer::IsSymbolCaption(const char *caption) noexcept
{
  if (caption == nullptr)
    return false;

  const char ch = caption[0];
  if (caption[1] != '\0')
    return false;

  return ch == '+' || ch == '-' || ch == '<' || ch == '>' ||
    ch == '^' || ch == 'v' || ch == 'h';
}

[[gnu::pure]]
static unsigned
MenuMaxDrawSize(const ButtonLook &look) noexcept
{
  if (look.font == nullptr)
    return 0;

  const PixelSize text = look.font->TextSize("+");
  const unsigned font_size = std::max(text.width, text.height);
  return std::max(1u, font_size / 2);
}

inline void
SymbolButtonRenderer::DrawSymbol(Canvas &canvas, PixelRect rc,
                                 ButtonState state) const noexcept
{
  const ButtonLook &look = GetLook();
  const unsigned max_draw_size = menu_scale ? MenuMaxDrawSize(look) : 0;

  canvas.SelectNullPen();

  switch (state) {
  case ButtonState::DISABLED:
    canvas.Select(look.disabled.brush);
    break;

  case ButtonState::FOCUSED:
  case ButtonState::PRESSED:
    canvas.Select(look.focused.foreground_brush);
    break;

  case ButtonState::SELECTED:
    canvas.Select(look.selected.foreground_brush);
    break;

  case ButtonState::ENABLED:
    canvas.Select(look.standard.foreground_brush);
    break;
  }

  const char ch = (char)caption[0u];

  // Draw arrow symbol instead of <
  if (ch == '<')
    SymbolRenderer::DrawArrow(canvas, rc, SymbolRenderer::LEFT,
                              max_draw_size);

  // Draw arrow symbol instead of >
  else if (ch == '>')
    SymbolRenderer::DrawArrow(canvas, rc, SymbolRenderer::RIGHT,
                              max_draw_size);

  // Draw arrow symbol instead of ^
  else if (ch == '^')
    SymbolRenderer::DrawArrow(canvas, rc, SymbolRenderer::UP,
                              max_draw_size);

  // Draw arrow symbol instead of v
  else if (ch == 'v')
    SymbolRenderer::DrawArrow(canvas, rc, SymbolRenderer::DOWN,
                              max_draw_size);

  // Draw symbols instead of + and -
  else if (ch == '+' || ch == '-')
    SymbolRenderer::DrawSign(canvas, rc, ch == '+', max_draw_size);

  // Draw hamburger menu icon (map overlay menu button)
  else if (ch == 'h')
    SymbolRenderer::DrawHamburger(canvas, rc);
}

void
SymbolButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                                 ButtonState state) const noexcept
{
  frame_renderer.DrawButton(canvas, rc, state);

  if (!caption.empty())
    DrawSymbol(canvas, frame_renderer.GetDrawingRect(rc, state),
               state);
}
