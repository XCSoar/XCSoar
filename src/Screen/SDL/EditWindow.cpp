/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Screen/EditWindow.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Features.hpp"
#include "Screen/Layout.hpp"

void
EditWindow::set(ContainerWindow &parent, PixelScalar left, PixelScalar top,
                UPixelScalar width, UPixelScalar height,
                const EditWindowStyle style)
{
  read_only = style.is_read_only;

  Window::set(&parent, left, top, width, height, style);
}

void
EditWindow::set(ContainerWindow &parent, const PixelRect rc,
                const EditWindowStyle style)
{
  read_only = style.is_read_only;

  Window::set(&parent, rc, style);
}

void
EditWindow::OnPaint(Canvas &canvas)
{
  if (is_enabled()) {
    if (is_read_only())
      canvas.clear(Color(0xf0, 0xf0, 0xf0));
    else
      canvas.ClearWhite();
    canvas.SetTextColor(COLOR_BLACK);
  } else {
    canvas.clear(COLOR_LIGHT_GRAY);
    canvas.SetTextColor(COLOR_DARK_GRAY);
  }

  PixelRect rc = {
    0, 0, PixelScalar(canvas.get_width() - 1),
    PixelScalar(canvas.get_height() - 1),
  };

  canvas.DrawOutlineRectangle(rc.left, rc.top, rc.right, rc.bottom, COLOR_BLACK);

  if (value.empty())
    return;

  canvas.SetBackgroundTransparent();

  PixelScalar padding = Layout::FastScale(2);
  InflateRect(&rc, -padding, -padding);

  if (have_clipping() || (get_text_style() & DT_WORDBREAK) != 0)
    canvas.formatted_text(&rc, value.c_str(), get_text_style());
  else if ((get_text_style() & DT_VCENTER) == 0)
    canvas.TextAutoClipped(rc.left, rc.top, value.c_str());
  else {
    PixelScalar canvas_height = rc.bottom - rc.top;
    UPixelScalar text_height = canvas.CalcTextHeight(value.c_str());
    PixelScalar top = rc.top + (canvas_height - text_height) / 2;
    canvas.TextAutoClipped(rc.left, top, value.c_str());
  }
}

void
EditWindow::set_text(const TCHAR *text)
{
  assert_none_locked();

  if (text != NULL)
    value = text;
  else
    value.clear();
  Invalidate();
}
