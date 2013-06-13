/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Form/SymbolButton.hpp"
#include "Form/Control.hpp"
#include "Formatter/HexColor.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/SymbolRenderer.hpp"
#include "resource.h"

void
WndSymbolButton::OnPaint(Canvas &canvas)
{
  const bool pressed = IsDown();
  const bool focused = HasCursorKeys() ? HasFocus() : pressed;

  PixelRect rc = canvas.GetRect();
  renderer.DrawButton(canvas, rc, focused, pressed);
  // If button has text on it
  const tstring caption = GetText();
  if (caption.empty())
    return;

  rc = renderer.GetDrawingRect(rc, pressed);

  canvas.SelectNullPen();
  if (!IsEnabled())
    canvas.Select(look.button.disabled.brush);
  else if (focused)
    canvas.Select(look.button.focused.foreground_brush);
  else
    canvas.Select(look.button.standard.foreground_brush);

  const char ch = (char)caption[0];

  Color color;

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
  else if (ch == '^' || ch == 'v')
    SymbolRenderer::DrawArrow(canvas, rc, SymbolRenderer::DOWN);

  // Draw symbols instead of + and -
  else if (ch == '+' || ch == '-')
    SymbolRenderer::DrawSign(canvas, rc, ch == '+');

  // Draw Fly bitmap
  else if (caption == _T("Fly")) {
    Bitmap launcher1_bitmap(IDB_LAUNCHER1);
    launcher1_bitmap.EnableInterpolation();
    canvas.ClearWhite();
    if (pressed)
      canvas.InvertStretchTransparent(launcher1_bitmap, COLOR_YELLOW);
    else
      canvas.StretchTransparent(launcher1_bitmap, COLOR_BLUE);
  }

  // Draw Simulator bitmap
  else if (caption == _T("Simulator")) {
    Bitmap launcher2_bitmap(IDB_LAUNCHER2);
    launcher2_bitmap.EnableInterpolation();
    canvas.ClearWhite();
    if (pressed)
      canvas.InvertStretchTransparent(launcher2_bitmap, COLOR_YELLOW);
    else
      canvas.StretchTransparent(launcher2_bitmap, COLOR_BLUE);
  }

  else if (ParseHexColor(caption.c_str(), color)) {
    rc.Grow(-3);
    canvas.DrawFilledRectangle(rc, color);
  }
}
