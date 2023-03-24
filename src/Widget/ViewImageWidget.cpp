// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
  void OnPaint(Canvas &canvas) noexcept override;
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
ViewImageWindow::OnPaint(Canvas &canvas) noexcept
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
