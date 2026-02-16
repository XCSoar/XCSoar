// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VScrollPanel.hpp"
#include "Look/DialogLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "Asset.hpp"
#include "Screen/Layout.hpp"
#include "Math/Point2D.hpp"
#include "util/StringAPI.hxx"

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

VScrollPanel::~VScrollPanel() noexcept = default;

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

int
VScrollPanel::GetScrollStep() const noexcept
{
  const int step = scroll_bar.GetWidth();
  return step > 0 ? step : 1;
}

void
VScrollPanel::ScrollBy(int delta) noexcept
{
  if (!scroll_bar.IsDefined())
    return;

  const unsigned physical_height = GetSize().height;
  if (virtual_height <= physical_height)
    return;

  const int max_origin =
    static_cast<int>(virtual_height) - static_cast<int>(physical_height);

  // If already animating, start from target; otherwise from current position
  int current = smooth_scroll_target >= 0 ? smooth_scroll_target
                                          : static_cast<int>(origin);
  int new_origin = current + delta;

  if (new_origin < 0)
    new_origin = 0;
  else if (new_origin > max_origin)
    new_origin = max_origin;

  if (new_origin == current)
    return;

  SmoothScrollTo(new_origin);
}

void
VScrollPanel::SmoothScrollTo(int target) noexcept
{
  const unsigned physical_height = GetSize().height;
  const int max_origin = std::max(0, static_cast<int>(virtual_height) -
                                     static_cast<int>(physical_height));

  smooth_scroll_target = std::clamp(target, 0, max_origin);

  // Start animation at ~60fps
  smooth_scroll_timer.Schedule(std::chrono::milliseconds(16));
}

void
VScrollPanel::OnSmoothScrollTimer() noexcept
{
  if (smooth_scroll_target < 0) {
    smooth_scroll_timer.Cancel();
    return;
  }

  const int diff = smooth_scroll_target - static_cast<int>(origin);

  if (std::abs(diff) <= 1) {
    // Close enough - snap to target and stop
    SetOriginClamped(smooth_scroll_target);
    smooth_scroll_target = -1;
    smooth_scroll_timer.Cancel();
  } else {
    // Ease-out: move a fraction of remaining distance
    // Using 1/3 gives ~100ms animation for typical distances
    const int step = diff / 3;
    SetOriginClamped(static_cast<int>(origin) + (step != 0 ? step : (diff > 0 ? 1 : -1)));
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
  smooth_scroll_timer.Cancel();
  PanelControl::OnDestroy();
}

bool
VScrollPanel::OnKeyCheck(unsigned key_code) const noexcept
{
  switch (key_code) {
  case KEY_UP:
  case KEY_DOWN:
  case KEY_PRIOR:
  case KEY_NEXT:
  case KEY_HOME:
  case KEY_END:
    return scroll_bar.IsDefined();
  }

  return PanelControl::OnKeyCheck(key_code);
}

bool
VScrollPanel::OnKeyDown(unsigned key_code) noexcept
{
  scroll_bar.DragEnd(this);

  const int step = GetScrollStep();
  const int page = std::max(1, static_cast<int>(GetSize().height) - step);

  switch (key_code) {
  case KEY_UP:
    ScrollBy(-step);
    return true;

  case KEY_DOWN:
    ScrollBy(step);
    return true;

  case KEY_PRIOR: // Page Up
    ScrollBy(-page);
    return true;

  case KEY_NEXT: // Page Down
    ScrollBy(page);
    return true;

  case KEY_HOME:
    SetOriginClamped(0);
    return true;

  case KEY_END:
    SetOriginClamped(static_cast<int>(virtual_height));
    return true;
  }

  return PanelControl::OnKeyDown(key_code);
}

bool
VScrollPanel::OnMouseUp(PixelPoint p) noexcept
{
  /* Finish gesture tracking and check for horizontal swipe */
  const char *gesture =
    gesture_tracking ? gestures.Finish() : nullptr;
  gesture_tracking = false;

  if (gesture != nullptr &&
      (StringIsEqual(gesture, "L") ||
       StringIsEqual(gesture, "R"))) {
    /* Horizontal swipe detected - forward to listener */
    if (dragging) {
      dragging = false;
      ReleaseCapture();
    }
    if (potential_tap) {
      potential_tap = false;
      ReleaseCapture();
    }
    if (listener.OnVScrollPanelGesture(gesture))
      return true;
  }

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

  if (potential_tap) {
    potential_tap = false;
    ReleaseCapture();
    return PanelControl::OnMouseUp(p);
  }

  return PanelControl::OnMouseUp(p);
}

bool
VScrollPanel::OnMouseMove(PixelPoint p, unsigned keys) noexcept
{
  if (gesture_tracking)
    gestures.Update(p);

  if (scroll_bar.IsDragging()) {
    origin = scroll_bar.DragMove(virtual_height, GetSize().height, p.y);
    listener.OnVScrollPanelChange();
    Invalidate();
    return true;
  }

  if (potential_tap) {
    const unsigned threshold = Layout::Scale(HasTouchScreen() ? 50 : 10);
    if ((unsigned)ManhattanDistance(drag_start, p) > threshold) {
      potential_tap = false;
      dragging = true;
      drag_y = (int)origin + drag_start.y;
      if (UsePixelPan())
        kinetic.MouseDown(origin);
    } else
      return PanelControl::OnMouseMove(p, keys);
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
  smooth_scroll_timer.Cancel();
  smooth_scroll_target = -1;

  if (scroll_bar.IsInsideSlider(p)) {
    scroll_bar.DragBegin(this, p.y);
    return true;
  } else if (scroll_bar.IsInside(p)) {
    /* click in the scroll bar area (arrows or track) */
    if (scroll_bar.IsInsideUpArrow(p.y)) {
      ScrollBy(-GetScrollStep());
    } else if (scroll_bar.IsInsideDownArrow(p.y)) {
      ScrollBy(GetScrollStep());
    } else if (scroll_bar.IsAboveSlider(p.y)) {
      /* page up */
      ScrollBy(-std::max(1,
        static_cast<int>(GetSize().height) - GetScrollStep()));
    } else if (scroll_bar.IsBelowSlider(p.y)) {
      /* page down */
      ScrollBy(std::max(1,
        static_cast<int>(GetSize().height) - GetScrollStep()));
    }
    return true;
  } else {
    /* Start gesture tracking for swipe detection only in the
       content area — not on the scrollbar, where slight horizontal
       finger movement during a tap would misfire as a page-change
       swipe (especially noticeable on e-ink touch screens). */
    gesture_tracking = true;
    gestures.Start(p, Layout::Scale(20));

    // First, let child widgets handle the event
    if (PanelControl::OnMouseDown(p)) {
      potential_tap = true;
      drag_start = p;
      SetCapture();
      return true;
    }

    // No child widget handled it, so start dragging the content area
    dragging = true;
    drag_y = (int)origin + p.y;
    if (UsePixelPan())
      kinetic.MouseDown(origin);
    SetCapture();
    return true;
  }
}

bool
VScrollPanel::OnMouseWheel(PixelPoint p, int delta) noexcept
{
  scroll_bar.DragEnd(this);

  if (PanelControl::OnMouseWheel(p, delta))
    return true;

  // Scroll by 3 steps per wheel notch (delta is typically 120 per notch)
  const int step = GetScrollStep();
  const int scroll_amount = (delta * 3 * step) / 120;
  ScrollBy(-scroll_amount);
  return true;
}

void
VScrollPanel::OnCancelMode() noexcept
{
  PanelControl::OnCancelMode();

  gesture_tracking = false;
  scroll_bar.DragEnd(this);
  if (dragging) {
    dragging = false;
    ReleaseCapture();
  }
  if (potential_tap) {
    potential_tap = false;
    ReleaseCapture();
  }
  kinetic_timer.Cancel();
  smooth_scroll_timer.Cancel();
  smooth_scroll_target = -1;
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
    scroll_bar.Paint(canvas, ButtonState::ENABLED, ButtonState::ENABLED);
  }
}

void
VScrollPanel::ScrollTo(const PixelRect &rc) noexcept
{
  if (scroll_bar.IsDefined()) {
    const unsigned physical_height = GetSize().height;
    if (physical_height >= virtual_height) {
      PanelControl::ScrollTo(rc);
      return;
    }

    // Calculate the target origin to bring rc into view
    int target_origin = static_cast<int>(origin);

    if (int delta = rc.bottom - static_cast<int>(physical_height); delta > 0)
      target_origin = std::min(target_origin + delta,
                               static_cast<int>(virtual_height - physical_height));

    if (int delta = rc.top; delta < 0)
      target_origin = std::max(target_origin + delta, 0);

    if (target_origin != static_cast<int>(origin))
      SmoothScrollTo(target_origin);
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
