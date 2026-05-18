// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ViewImageWidget.hpp"
#include "ImageZoomFrame.hpp"
#include "ui/canvas/Bitmap.hpp"

void
ViewImageWidget::SetBitmap(const Bitmap *_bitmap) noexcept
{
  bitmap = _bitmap;

  if (!IsDefined())
    return;

  auto &frame = (ImageZoomFrame &)GetWindow();
  frame.SetContent(bitmap, &zoom);
  frame.Invalidate();
}

void
ViewImageWidget::Prepare(ContainerWindow &parent,
                         const PixelRect &rc) noexcept
{
  WindowStyle hidden;
  hidden.Hide();

  auto frame = std::make_unique<ImageZoomFrame>();
  frame->Create(parent, rc, hidden);
  frame->SetContent(bitmap, &zoom);
  SetWindow(std::move(frame));
}
