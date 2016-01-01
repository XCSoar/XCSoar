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

#include "ViewImageWidget.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Bitmap.hpp"

void
ViewImageWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  WindowStyle hidden;
  hidden.Hide();

  view.Create(parent, rc, hidden,
              [this](Canvas &canvas, const PixelRect &rc){
                OnImagePaint(canvas, rc);
              });
  SetWindow(&view);
}

void
ViewImageWidget::Unprepare()
{
  view.Destroy();
}

void
ViewImageWidget::OnImagePaint(Canvas &canvas, const PixelRect &rc)
{
  canvas.ClearWhite();

  const PixelSize bitmap_size = bitmap.GetSize();
  const PixelSize window_size(rc.right - rc.left, rc.bottom - rc.top);

  PixelSize fit_size(window_size.cx,
                     window_size.cx * bitmap_size.cy / bitmap_size.cx);
  if (fit_size.cy > window_size.cy) {
    fit_size.cy = window_size.cy;
    fit_size.cx = window_size.cy * bitmap_size.cx / bitmap_size.cy;
  }

  canvas.Stretch((rc.left + rc.right - fit_size.cx) / 2,
                 (rc.top + rc.bottom - fit_size.cy) / 2,
                 fit_size.cx, fit_size.cy,
                 bitmap);
}
