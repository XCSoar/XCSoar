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
#include "Asset.hpp"
#include "Hardware/CPU.hpp"

#include <cmath>

/**
 * Can the image glide on after a drag?  Fast displays get kinetic
 * panning; e-paper and slow CPUs stop with the finger (same gate as
 * the list and the map pan animations).
 */
[[gnu::pure]]
static bool
UseKineticPan() noexcept
{
  return !HasEPaper() && !IsSlowCPU();
}

void
ImageZoomFrame::Create(ContainerWindow &parent, const PixelRect rc,
                       const WindowStyle &style) noexcept
{
  PaintWindow::Create(parent, rc, style);
}

void
ImageZoomFrame::SetContent(const Bitmap *_bitmap, double *_zoom_factor) noexcept
{
  kinetic_timer.Cancel();

  bitmap = _bitmap;
  zoom_factor = _zoom_factor;
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
ImageZoomFrame::SetOnZoomChanged(std::function<void()> &&f) noexcept
{
  on_zoom_changed = std::move(f);
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

  if (bitmap == nullptr || zoom_factor == nullptr)
    return;

  ImageZoomView::PaintZoomedBitmap(canvas, *bitmap, *zoom_factor,
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
  drag_moved = true;

  kinetic_x.MouseMove(p.x);
  kinetic_y.MouseMove(p.y);

  Invalidate();
  return true;
}

bool
ImageZoomFrame::OnMouseDown(const PixelPoint p) noexcept
{
  kinetic_timer.Cancel();

  is_dragging = true;
  drag_moved = false;
  last_mouse_pos = p;

  kinetic_x.MouseDown(p.x);
  kinetic_y.MouseDown(p.y);

  /* capture the pointer, so the drag continues outside this window,
     and so multi-touch events are delivered here */
  SetCapture();
  return true;
}

bool
ImageZoomFrame::OnMouseUp(const PixelPoint p) noexcept
{
  if (is_dragging && drag_moved)
    StartKineticPan(p);

  is_dragging = false;
#ifdef HAVE_MULTI_TOUCH
  is_pinching = false;
#endif
  ReleaseCapture();
  return true;
}

void
ImageZoomFrame::StartKineticPan(const PixelPoint p) noexcept
{
  if (!UseKineticPan())
    return;

  kinetic_x.MouseUp(p.x);
  kinetic_y.MouseUp(p.y);

  if (kinetic_x.IsSteady() && kinetic_y.IsSteady())
    /* the finger rested before it was lifted */
    return;

  /* the kinetic managers extrapolate from their own last filtered
     position, which may differ from p; start from what they report
     now, so that the glide continues without a jump */
  kinetic_last = {kinetic_x.GetPosition(), kinetic_y.GetPosition()};

  kinetic_timer.Schedule(std::chrono::milliseconds(30));
}

void
ImageZoomFrame::OnKineticTimer() noexcept
{
  if (kinetic_x.IsSteady() && kinetic_y.IsSteady()) {
    kinetic_timer.Cancel();
    return;
  }

  const PixelPoint p{kinetic_x.GetPosition(), kinetic_y.GetPosition()};
  if (p == kinetic_last)
    return;

  pending_offset += kinetic_last - p;
  kinetic_last = p;
  Invalidate();
}

#ifdef HAVE_MULTI_TOUCH

bool
ImageZoomFrame::OnMultiTouchDown() noexcept
{
  if (bitmap == nullptr || zoom_factor == nullptr)
    return false;

  kinetic_timer.Cancel();

  /* the second finger takes over: a single-finger drag must not pan
     while the pinch is running */
  is_dragging = false;
  is_pinching = false;
  return true;
}

bool
ImageZoomFrame::OnMultiTouchMove(const PixelPoint a,
                                 const PixelPoint b) noexcept
{
  if (bitmap == nullptr || zoom_factor == nullptr)
    return false;

  const double distance = std::hypot(double(a.x - b.x), double(a.y - b.y));
  if (distance < 1)
    return true;

  const PixelPoint centroid{(a.x + b.x) / 2, (a.y + b.y) / 2};

  if (!is_pinching) {
    is_dragging = false;
    is_pinching = true;
    pinch_distance = distance;
    pinch_zoom_factor = *zoom_factor;
    pinch_last_a = a;
    pinch_last_b = b;

    /* remember which bitmap position the fingers grabbed; it stays
       below them for the rest of the gesture */
    pinch_anchor = ImageZoomView::CanvasToBitmap(centroid, view_pos,
                                                 GetSize(),
                                                 bitmap->GetSize(),
                                                 *zoom_factor);

    /* the gesture writes the view position directly, and a pan offset
       which has not been painted yet would be applied on top of it */
    pending_offset = {};
    return true;
  }

  if (a == pinch_last_a && b == pinch_last_b)
    /* one motion event per finger arrives, but both positions are read
       from the current device state; skip the duplicate */
    return true;

  pinch_last_a = a;
  pinch_last_b = b;

  /* scale with the distance between the fingers, and keep the grabbed
     bitmap position below their centre; this zooms and pans in one
     step */
  *zoom_factor = ImageZoomView::ClampZoomFactor(pinch_zoom_factor *
                                                distance / pinch_distance);

  ImageZoomView::MoveViewTo(pinch_anchor, centroid, view_pos, GetSize(),
                            bitmap->GetSize(), *zoom_factor);

  if (on_zoom_changed)
    on_zoom_changed();

  Invalidate();
  return true;
}

bool
ImageZoomFrame::OnMultiTouchUp() noexcept
{
  if (!is_pinching)
    return false;

  /* keep the current zoom factor; the remaining finger does not resume
     panning, because that would jump the image */
  is_pinching = false;
  return true;
}

#endif /* HAVE_MULTI_TOUCH */

void
ImageZoomFrame::OnCancelMode() noexcept
{
  is_dragging = false;
#ifdef HAVE_MULTI_TOUCH
  is_pinching = false;
#endif
  kinetic_timer.Cancel();

  WndOwnerDrawFrame::OnCancelMode();
}

void
ImageZoomFrame::OnDestroy() noexcept
{
  kinetic_timer.Cancel();

  WndOwnerDrawFrame::OnDestroy();
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
  kinetic_timer.Cancel();

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
