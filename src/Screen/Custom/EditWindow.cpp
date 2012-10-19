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
EditWindow::Create(ContainerWindow &parent, PixelRect rc,
                   const EditWindowStyle style)
{
  read_only = style.is_read_only;

  Window::Create(&parent, rc, style);
}

void
EditWindow::OnPaint(Canvas &canvas)
{
  if (IsEnabled()) {
    if (IsReadOnly())
      canvas.Clear(Color(0xf0, 0xf0, 0xf0));
    else
      canvas.ClearWhite();
    canvas.SetTextColor(COLOR_BLACK);
  } else {
    canvas.Clear(COLOR_LIGHT_GRAY);
    canvas.SetTextColor(COLOR_DARK_GRAY);
  }

  PixelRect rc = {
    0, 0, PixelScalar(canvas.GetWidth() - 1),
    PixelScalar(canvas.GetHeight() - 1),
  };

  canvas.DrawOutlineRectangle(rc.left, rc.top, rc.right, rc.bottom, COLOR_BLACK);

  if (value.empty())
    return;

  canvas.SetBackgroundTransparent();

  const PixelScalar x = Layout::GetTextPadding();
  const PixelScalar canvas_height = canvas.GetHeight();
  const PixelScalar text_height = canvas.GetFontHeight();
  const PixelScalar y = (canvas_height - text_height) / 2;

  canvas.TextAutoClipped(x, y, value.c_str());
}

void
EditWindow::SetText(const TCHAR *text)
{
  AssertNoneLocked();

  if (text != NULL)
    value = text;
  else
    value.clear();
  Invalidate();
}
