// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VScrollPanel.hpp"
#include "Look/DialogLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "Asset.hpp"
#include "Screen/Layout.hpp"
#include "Math/Point2D.hpp"

#include <algorithm>

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#endif

class VScrollPanel::ScrollBarButton final : public PaintWindow {
  VScrollPanel &panel;
  int direction;
  bool pressed = false;

public:
  ScrollBarButton(VScrollPanel &_panel, int _direction) noexcept
    :panel(_panel), direction(_direction) {}

  void Create(VScrollPanel &parent) noexcept {
    WindowStyle style;
    style.Hide();
    // No TabStop - scrollbar buttons are clickable but not in tab order.
    // Keyboard users scroll via Up/Down/PageUp/PageDown on the content.
    PaintWindow::Create(parent, PixelRect{0, 0, 1, 1}, style);
  }

protected:
  bool OnMouseDown([[maybe_unused]] PixelPoint p) noexcept override {
    pressed = true;
    SetCapture();
    return true;
  }

  bool OnMouseUp(PixelPoint p) noexcept override {
    if (!pressed)
      return true;

    pressed = false;
    ReleaseCapture();

    if (IsInside(p))
      panel.ScrollBy(direction * panel.GetScrollStep());

    return true;
  }

  void OnCancelMode() noexcept override {
    PaintWindow::OnCancelMode();

    if (pressed) {
      pressed = false;
      ReleaseCapture();
    }
  }

  // Parent panel draws scrollbar visuals via scroll_bar.Paint(); this window
  // only handles input/focus for the scrollbar, so OnPaint is intentionally
  // a no-op.
  void OnPaint([[maybe_unused]] Canvas &canvas) noexcept override {}
};

VScrollPanel::VScrollPanel(ContainerWindow &parent, const DialogLook &look,
                           const PixelRect &rc,
                           const WindowStyle style,
                           VScrollPanelListener &_listener) noexcept
  :PanelControl(parent, look, rc, style),
   listener(_listener),
   scroll_bar(look.button)
{
  up_button = std::make_unique<ScrollBarButton>(*this, -1);
  down_button = std::make_unique<ScrollBarButton>(*this, 1);
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

  UpdateArrowButtons();
}

void
VScrollPanel::UpdateArrowButtons() noexcept
{
  if (!scroll_bar.IsDefined()) {
    up_arrow_rect.SetEmpty();
    down_arrow_rect.SetEmpty();
    if (up_button->IsDefined())
      up_button->Hide();
    if (down_button->IsDefined())
      down_button->Hide();
    return;
  }

  if (!up_button->IsDefined())
    up_button->Create(*this);
  if (!down_button->IsDefined())
    down_button->Create(*this);

  const PixelSize size = GetSize();
  const int width = scroll_bar.GetWidth();
  const int left = static_cast<int>(size.width) - width;

  up_arrow_rect = PixelRect{left, 0, static_cast<int>(size.width), width};
  down_arrow_rect = PixelRect{left, static_cast<int>(size.height) - width,
                              static_cast<int>(size.width),
                              static_cast<int>(size.height)};

  up_button->MoveAndShow(up_arrow_rect);
  down_button->MoveAndShow(down_arrow_rect);
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
  int new_origin = static_cast<int>(origin) + delta;
  if (new_origin < 0)
    new_origin = 0;
  else if (new_origin > max_origin)
    new_origin = max_origin;

  if (static_cast<unsigned>(new_origin) == origin)
    return;

  origin = static_cast<unsigned>(new_origin);
  listener.OnVScrollPanelChange();
  Invalidate();
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
  if (scroll_bar.IsDragging()) {
    origin = scroll_bar.DragMove(virtual_height, GetSize().height, p.y);
    listener.OnVScrollPanelChange();
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

  if (scroll_bar.IsInsideSlider(p)) {
    scroll_bar.DragBegin(this, p.y);
    return true;
  } else if (!scroll_bar.IsInside(p)) {
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
  } else
    return PanelControl::OnMouseDown(p);
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
