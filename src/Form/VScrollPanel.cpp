// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VScrollPanel.hpp"
#include "Look/DialogLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#endif

class VScrollPanel::ScrollBarButton final : public PaintWindow {
  VScrollPanel &panel;
  int direction;
  bool pressed = false;
  bool focus_on_click = false;

public:
  ScrollBarButton(VScrollPanel &_panel, int _direction) noexcept
    :panel(_panel), direction(_direction) {}

  void Create(VScrollPanel &parent) noexcept {
    WindowStyle style;
    style.Hide();
    style.TabStop();
    PaintWindow::Create(parent, PixelRect{0, 0, 1, 1}, style);
  }

protected:
  bool OnKeyCheck(unsigned key_code) const noexcept override {
    return key_code == KEY_RETURN || key_code == KEY_SPACE;
  }

  bool OnKeyDown(unsigned key_code) noexcept override {
    switch (key_code) {
    case KEY_RETURN:
    case KEY_SPACE:
      panel.ScrollBy(direction * panel.GetScrollStep());
      return true;
    }

    return PaintWindow::OnKeyDown(key_code);
  }

  bool OnMouseDown([[maybe_unused]] PixelPoint p) noexcept override {
    if (IsTabStop() && !HasFocus()) {
      SetFocus();
      focus_on_click = true;
    } else {
      focus_on_click = false;
    }

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

    if (focus_on_click)
      FocusParent();

    return true;
  }

  void OnCancelMode() noexcept override {
    PaintWindow::OnCancelMode();

    if (pressed) {
      pressed = false;
      ReleaseCapture();
    }
  }

  void OnSetFocus() noexcept override {
    PaintWindow::OnSetFocus();
    panel.Invalidate();
  }

  void OnKillFocus() noexcept override {
    PaintWindow::OnKillFocus();
    panel.Invalidate();
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

void
VScrollPanel::OnResize(PixelSize new_size) noexcept
{
  PanelControl::OnResize(new_size);
  SetupScrollBar();
  listener.OnVScrollPanelChange();
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

  return PanelControl::OnMouseMove(p, keys);
}

bool
VScrollPanel::OnMouseDown(PixelPoint p) noexcept
{
  scroll_bar.DragEnd(this);

  if (scroll_bar.IsInsideSlider(p)) {
    scroll_bar.DragBegin(this, p.y);
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
    const ButtonState up_state = up_button->HasFocus()
      ? ButtonState::FOCUSED
      : ButtonState::ENABLED;
    const ButtonState down_state = down_button->HasFocus()
      ? ButtonState::FOCUSED
      : ButtonState::ENABLED;
    scroll_bar.Paint(canvas, up_state, down_state);
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
