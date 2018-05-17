/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "SymbolButtonRenderer.hpp"
#include "SymbolRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Look/ButtonLook.hpp"

inline void
SymbolButtonRenderer::DrawSymbol(Canvas &canvas, PixelRect rc, bool enabled,
                                 bool focused, bool pressed) const
{
  const ButtonLook &look = GetLook();

  canvas.SelectNullPen();
  if (!enabled)
    canvas.Select(look.disabled.brush);
  else if (focused)
    canvas.Select(look.focused.foreground_brush);
  else
    canvas.Select(look.standard.foreground_brush);

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
                                 bool enabled,
                                 bool focused, bool pressed) const
{
  frame_renderer.DrawButton(canvas, rc, focused, pressed);

  if (!caption.empty())
    DrawSymbol(canvas, frame_renderer.GetDrawingRect(rc, pressed),
               enabled, focused, pressed);
}
