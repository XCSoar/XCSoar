// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VScrollPanel.hpp"
#include "Look/DialogLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Asset.hpp"

#include <algorithm>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#endif

VScrollPanel::VScrollPanel(ContainerWindow &parent, const DialogLook &look,
                           const PixelRect &rc,
                           const WindowStyle style,
                           VScrollPanelListener &_listener) noexcept
  :PanelControl(parent, look, rc, style),
   listener(_listener),
   scroll_bar(look.button)
{
}

void
VScrollPanel::SetVirtualHeight(unsigned _virtual_height) noexcept
{
  virtual_height = _virtual_height;

  SetupScrollBar();
}

void
VScrollPanel::SetupScrollBar() noexcept
{
  const auto size = GetSize();
  if (virtual_height > size.height) {
    if (origin + size.height > virtual_height)
      origin = virtual_height - size.height;

    scroll_bar.SetSize(size);
  } else {
    origin = 0;
    scroll_bar.Reset();
  }
}

static bool UsePixelPan() noexcept
{
  return !HasEPaper();
}

void
VScrollPanel::SetOriginClamped(int new_origin) noexcept
{
  const int max_origin = std::max(0, int(virtual_height) - int(GetSize().height));
  if (new_origin < 0)
    new_origin = 0;
  else if (new_origin > max_origin)
    new_origin = max_origin;

  if ((unsigned)new_origin == origin)
    return;

  origin = (unsigned)new_origin;
  listener.OnVScrollPanelChange();
  Invalidate();
}

void
VScrollPanel::OnResize(PixelSize new_size) noexcept
{
  PanelControl::OnResize(new_size);
  SetupScrollBar();
  listener.OnVScrollPanelChange();
}

void
VScrollPanel::OnDestroy() noexcept
{
  kinetic_timer.Cancel();
  PanelControl::OnDestroy();
}

bool
VScrollPanel::OnKeyDown(unsigned key_code) noexcept
{
  scroll_bar.DragEnd(this);

  return PanelControl::OnKeyDown(key_code);
}

bool
VScrollPanel::OnMouseUp(PixelPoint p) noexcept
{
  if (scroll_bar.IsDragging()) {
    scroll_bar.DragEnd(this);
    return true;
  }

  if (dragging) {
    const bool enable_kinetic = UsePixelPan();

    dragging = false;
    ReleaseCapture();

    if (enable_kinetic) {
      kinetic.MouseUp(origin);
      kinetic_timer.Schedule(std::chrono::milliseconds(30));
    }
    return true;
  }

  return PanelControl::OnMouseUp(p);
}

bool
VScrollPanel::OnMouseMove(PixelPoint p, unsigned keys) noexcept
{
  if (scroll_bar.IsDragging()) {
    origin = scroll_bar.DragMove(virtual_height, GetSize().height, p.y);
    listener.OnVScrollPanelChange();
    return true;
  }

  if (dragging) {
    int new_origin = drag_y - p.y;
    SetOriginClamped(new_origin);
    if (UsePixelPan())
      kinetic.MouseMove(origin);
    return true;
  }

  return PanelControl::OnMouseMove(p, keys);
}

bool
VScrollPanel::OnMouseDown(PixelPoint p) noexcept
{
  scroll_bar.DragEnd(this);

  kinetic_timer.Cancel();

  if (scroll_bar.IsInsideSlider(p)) {
    scroll_bar.DragBegin(this, p.y);
    return true;
  } else if (!scroll_bar.IsInside(p)) {
    // Start dragging the content area itself instead of the scroll bar
    dragging = true;
    drag_y = (int)origin + p.y;
    if (UsePixelPan())
      kinetic.MouseDown(origin);
    SetCapture();
    return true;
  } else
    return PanelControl::OnMouseDown(p);
}

bool
VScrollPanel::OnMouseWheel(PixelPoint p, int delta) noexcept
{
  scroll_bar.DragEnd(this);

  if (PanelControl::OnMouseWheel(p, delta))
    return true;

  // TODO move origin
  return false;
}

void
VScrollPanel::OnCancelMode() noexcept
{
  PanelControl::OnCancelMode();

  scroll_bar.DragEnd(this);
  if (dragging) {
    dragging = false;
    ReleaseCapture();
  }
  kinetic_timer.Cancel();
}

void
VScrollPanel::OnPaint(Canvas &canvas) noexcept
{
  {
#ifdef ENABLE_OPENGL
    /* enable clipping */
    const GLCanvasScissor scissor{GetPhysicalRect(canvas.GetSize())};
#endif

    PanelControl::OnPaint(canvas);
  }

  if (scroll_bar.IsDefined()) {
    scroll_bar.SetSlider(virtual_height, canvas.GetHeight(), origin);
    scroll_bar.Paint(canvas);
  }
}

void
VScrollPanel::ScrollTo(const PixelRect &rc) noexcept
{
  if (scroll_bar.IsDefined()) {
    const unsigned old_origin = origin;

    const unsigned physical_height = GetSize().height;
    assert(physical_height < virtual_height);

    if (int delta = rc.bottom - (int)physical_height; delta > 0)
      origin = std::min(origin + (unsigned)delta,
                        virtual_height - (unsigned)physical_height);

    if (int delta = rc.top; delta < 0)
      origin = std::max((int)origin + delta, 0);

    if (origin != old_origin)
      listener.OnVScrollPanelChange();
  }

  PanelControl::ScrollTo(rc);
}

void
VScrollPanel::OnKineticTimer() noexcept
{
  assert(UsePixelPan());

  if (kinetic.IsSteady()) {
    kinetic_timer.Cancel();
    return;
  }

  SetOriginClamped(kinetic.GetPosition());
}
