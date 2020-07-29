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

#include "ButtonRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/ButtonLook.hpp"
#include "Util/Macros.hpp"

#include "Look/Themes/BlueTheme.hpp"

unsigned
ButtonFrameRenderer::GetMargin()
{
  return Layout::VptScale(2);
}

void ButtonFrameRenderer::DrawButton(Canvas &canvas, PixelRect rc,
                                     bool focused, bool pressed) const
{
  DrawButton(canvas, rc, focused, pressed, true);
}

void ButtonFrameRenderer::DrawButton(Canvas &canvas, PixelRect rc,
                                     bool focused, bool pressed, bool enabled) const
{
  //space around buttons
  int space = 1;

  int bevel_bottom = Layout::VptScale(3);
  int bevel_right = Layout::VptScale(3);

  PixelRect rectangle = PixelRect(rc.left + space, rc.top + space, rc.right - space - bevel_right, rc.bottom - space - bevel_bottom);
  PixelRect rectangle_bottom = PixelRect(rectangle.left, rectangle.bottom, rectangle.right, rc.bottom - space);
  PixelRect rectangle_right = PixelRect(rectangle.right, rc.top + space, rc.right - space, rc.bottom - space);

  //check if button is enabled
  if (enabled)
  {
    //check if button is pressed
    if (pressed)
    {
      canvas.DrawFilledRectangle(rectangle, look.pressed.background_color);
      canvas.DrawFilledRectangle(rectangle_bottom, look.pressed.background_color2);
      canvas.DrawFilledRectangle(rectangle_right, look.pressed.background_color2);
    }

    //check if button has focus
    else if (focused)
    {
      PixelRect focus_rect = PixelRect(rc.left + 5, rc.top + 5, rc.right - 5, rc.bottom - 5);
      canvas.DrawFilledRectangle(rc, look.focused.background_color2);
      canvas.DrawFilledRectangle(focus_rect, look.focused.background_color);
    }

    //normal button
    else
    {
      canvas.DrawFilledRectangle(rectangle, look.standard.background_color);
      canvas.DrawFilledRectangle(rectangle_bottom, look.standard.background_color2);
      canvas.DrawFilledRectangle(rectangle_right, look.standard.background_color2);
    }
  }

  //button is disabled
  else
  {
    canvas.DrawFilledRectangle(rectangle, look.disabled.background_color);
    canvas.DrawFilledRectangle(rectangle_bottom, look.disabled.background_color2);
    canvas.DrawFilledRectangle(rectangle_right, look.disabled.background_color2);
  }
}

PixelRect
ButtonFrameRenderer::GetDrawingRect(PixelRect rc, bool pressed) const
{
  rc.Grow(-2);
  if (pressed)
    rc.Offset(1, 1);

  return rc;
}

unsigned
ButtonRenderer::GetMinimumButtonWidth() const
{
  return Layout::GetMaximumControlHeight();
}
