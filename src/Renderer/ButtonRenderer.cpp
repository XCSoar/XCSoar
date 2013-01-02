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

#include "ButtonRenderer.hpp"
#include "Screen/Color.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Pen.hpp"
#include "Look/ButtonLook.hpp"

void
ButtonRenderer::DrawButton(Canvas &canvas, PixelRect rc, bool focused,
                           bool pressed)
{
  const ButtonLook::StateLook &_look = focused ? look.focused : look.standard;

  canvas.DrawFilledRectangle(rc, _look.background_color);

  canvas.Select(pressed ? _look.dark_border_pen : _look.light_border_pen);
  canvas.DrawTwoLines(rc.left, rc.bottom - 2, rc.left, rc.top, rc.right - 2,
                      rc.top);
  canvas.DrawTwoLines(rc.left + 1, rc.bottom - 3, rc.left + 1, rc.top + 1,
                      rc.right - 3, rc.top + 1);

  canvas.Select(pressed ? _look.light_border_pen : _look.dark_border_pen);
  canvas.DrawTwoLines(rc.left + 1, rc.bottom - 1, rc.right - 1, rc.bottom - 1,
                      rc.right - 1, rc.top + 1);
  canvas.DrawTwoLines(rc.left + 2, rc.bottom - 2, rc.right - 2, rc.bottom - 2,
                      rc.right - 2, rc.top + 2);
}

PixelRect
ButtonRenderer::GetDrawingRect(PixelRect rc, bool pressed)
{
  rc.Grow(-2);
  if (pressed)
    rc.Offset(1, 1);

  return rc;
}
