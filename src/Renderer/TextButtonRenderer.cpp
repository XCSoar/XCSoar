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

#include "TextButtonRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/ButtonLook.hpp"

inline void
TextButtonRenderer::DrawCaption(Canvas &canvas, const PixelRect &rc,
                                bool enabled, bool focused, bool pressed) const
{
  const ButtonLook &look = GetLook();

  canvas.SetBackgroundTransparent();
  if (!enabled)
    canvas.SetTextColor(look.disabled.color);
  else if (focused)
    canvas.SetTextColor(look.focused.foreground_color);
  else
    canvas.SetTextColor(look.standard.foreground_color);

  canvas.Select(*look.font);

  text_renderer.Draw(canvas, rc, GetCaption());
}

unsigned
TextButtonRenderer::GetMinimumButtonWidth() const
{
  return 2 * (frame_renderer.GetMargin() + Layout::GetTextPadding())
    + GetLook().font->TextSize(caption.c_str()).cx;
}

void
TextButtonRenderer::DrawButton(Canvas &canvas, const PixelRect &rc,
                               bool enabled, bool focused, bool pressed) const
{
  frame_renderer.DrawButton(canvas, rc, focused, pressed);

  if (!caption.empty())
    DrawCaption(canvas, frame_renderer.GetDrawingRect(rc, pressed),
                enabled, focused, pressed);
}

