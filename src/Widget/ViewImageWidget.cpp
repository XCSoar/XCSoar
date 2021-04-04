/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/window/PaintWindow.hpp"

class ViewImageWindow final : public PaintWindow {
  const Bitmap *bitmap;

public:
  explicit ViewImageWindow(const Bitmap *_bitmap):bitmap(_bitmap) {}

  void SetBitmap(const Bitmap *_bitmap) {
    bitmap = _bitmap;
    Invalidate();
  }

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) override;
};

void
ViewImageWidget::SetBitmap(const Bitmap *_bitmap)
{
  bitmap = _bitmap;

  if (IsDefined())
    ((ViewImageWindow &)GetWindow()).SetBitmap(_bitmap);
}

void
ViewImageWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle hidden;
  hidden.Hide();

  auto w = std::make_unique<ViewImageWindow>(bitmap);
  w->Create(parent, rc, hidden);
  SetWindow(std::move(w));
}

void
ViewImageWindow::OnPaint(Canvas &canvas)
{
  canvas.ClearWhite();

  if (bitmap == nullptr)
    return;

  const PixelSize bitmap_size = bitmap->GetSize();
  const PixelRect rc = GetClientRect();
  const PixelSize window_size = rc.GetSize();

  PixelSize fit_size(window_size.width,
                     window_size.width * bitmap_size.height / bitmap_size.width);
  if (fit_size.height > window_size.height) {
    fit_size.height = window_size.height;
    fit_size.width = window_size.height * bitmap_size.width / bitmap_size.height;
  }

  canvas.Stretch(rc.CenteredTopLeft(fit_size), fit_size, *bitmap);
}
