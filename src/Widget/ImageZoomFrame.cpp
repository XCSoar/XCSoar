// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ImageZoomFrame.hpp"
#include "ImageZoomView.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Features.hpp"
#include "ui/event/KeyCode.hpp"

void
ImageZoomFrame::Create(ContainerWindow &parent, const PixelRect rc,
                       const WindowStyle &style) noexcept
{
  PaintWindow::Create(parent, rc, style);
}

void
ImageZoomFrame::SetContent(const Bitmap *_bitmap, int *zoom) noexcept
{
  bitmap = _bitmap;
  zoom_level = zoom;
  view_pos = {};
  pending_offset = {};
  if (IsDefined())
    Invalidate();
}

void
ImageZoomFrame::SetTryKeyInput(std::function<bool(unsigned)> &&f) noexcept
{
  try_key_input = std::move(f);
}

void
ImageZoomFrame::NudgeViewByPixelOffset(const PixelPoint o) noexcept
{
  pending_offset += o;
  Invalidate();
}

void
ImageZoomFrame::OnPaint(Canvas &canvas) noexcept
{
  const auto &look = UIGlobals::GetDialogLook();
  if (HaveClipping())
    canvas.Clear(look.background_color);

  if (bitmap == nullptr || zoom_level == nullptr)
    return;

  ImageZoomView::PaintZoomedBitmap(canvas, *bitmap, *zoom_level,
                                   view_pos, pending_offset);
}

bool
ImageZoomFrame::OnMouseMove(const PixelPoint p,
                             [[maybe_unused]] unsigned keys) noexcept
{
  if (!is_dragging)
    return false;

  pending_offset += last_mouse_pos - p;
  last_mouse_pos = p;
  Invalidate();
  return true;
}

bool
ImageZoomFrame::OnMouseDown(const PixelPoint p) noexcept
{
  is_dragging = true;
  last_mouse_pos = p;
  return true;
}

bool
ImageZoomFrame::OnMouseUp([[maybe_unused]] const PixelPoint p) noexcept
{
  is_dragging = false;
  return true;
}

bool
ImageZoomFrame::OnKeyCheck(const unsigned key_code) const noexcept
{
  if (try_key_input) {
    switch (key_code) {
    case KEY_F2:
    case KEY_F3:
      return true;
    }
  }

  switch (key_code) {
  case KEY_LEFT:
  case KEY_RIGHT:
  case KEY_UP:
  case KEY_DOWN:
    return true;

  default:
    return false;
  }
}

bool
ImageZoomFrame::OnKeyDown(const unsigned key_code) noexcept
{
  if (try_key_input && try_key_input(key_code))
    return true;

  switch (key_code) {
  case KEY_LEFT:
    pending_offset.x -= 50;
    break;

  case KEY_RIGHT:
    pending_offset.x += 50;
    break;

  case KEY_UP:
    pending_offset.y -= 50;
    break;

  case KEY_DOWN:
    pending_offset.y += 50;
    break;

  default:
    return false;
  }

  Invalidate();
  return true;
}
