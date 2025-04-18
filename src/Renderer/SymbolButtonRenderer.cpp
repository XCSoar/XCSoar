// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SymbolButtonRenderer.hpp"
#include "SymbolRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/ButtonLook.hpp"

inline void
SymbolButtonRenderer::DrawSymbol(Canvas &canvas, PixelRect rc,
                                 ButtonState state) const noexcept
{
  const ButtonLook &look = GetLook();

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
    SymbolRenderer::DrawArrow(canvas, rc, SymbolRenderer::LEFT);

  // Draw arrow symbol instead of >
  else if (ch == '>')
    SymbolRenderer::DrawArrow(canvas, rc, SymbolRenderer::RIGHT);

  // Draw arrow symbol instead of ^
  else if (ch == '^')
    SymbolRenderer::DrawArrow(canvas, rc, SymbolRenderer::UP);

  // Draw arrow symbol instead of v
  else if (ch == 'v')
    SymbolRenderer::DrawArrow(canvas, rc, SymbolRenderer::DOWN);

  // Draw symbols instead of + and -
  else if (ch == '+' || ch == '-')
    SymbolRenderer::DrawSign(canvas, rc, ch == '+');
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
