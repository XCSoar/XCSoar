// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VScrollWidget.hpp"
#include "Asset.hpp"
#include "Form/Panel.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "util/StringAPI.hxx"

#include <algorithm>
#include <cassert>

unsigned
VScrollWidget::GetScrollbarWidth() noexcept
{
  return HasPointer()
    ? Layout::GetMinimumControlHeight()
    : Layout::VptScale(10);
}

PixelRect
VScrollWidget::AdjustForScrollbar(PixelRect rc) const noexcept
{
  if (!reserve_scrollbar)
    return rc;

  const unsigned scrollbar_width = GetScrollbarWidth();
  if (scrollbar_width > 0 && rc.GetWidth() > scrollbar_width)
    rc.right -= scrollbar_width;

  return rc;
}

inline unsigned
VScrollWidget::CalcVirtualHeight(const PixelRect &rc) const noexcept
{
  const unsigned height = rc.GetHeight();
  const unsigned max_height = widget->GetMaximumSize().height;

  if (reserve_scrollbar) {
    /* Rich-text / prose content: the widget has a fixed content
       height and cannot shrink, so scroll the full extent. */
    return std::max({1u, max_height, height});
  }

  /* Flexible form widgets: only scroll when the widget truly
     cannot compress to fit the viewport (min_height > height). */
  const unsigned virtual_height = max_height <= height
    ? max_height
    : std::max(widget->GetMinimumSize().height, height);

  /* Window::Move() requires a non-empty rectangle. */
  return std::max(1u, virtual_height);
}

inline void
VScrollWidget::UpdateVirtualHeight(const PixelRect &rc) noexcept
{
  GetWindow().SetVirtualHeight(CalcVirtualHeight(rc));
}

PixelSize
VScrollWidget::GetMinimumSize() const noexcept
{
  return widget->GetMinimumSize();
}

PixelSize
VScrollWidget::GetMaximumSize() const noexcept
{
  PixelSize size = widget->GetMaximumSize();

  if (maximum_layout_height > 0) {
    const unsigned minimum_height = widget->GetMinimumSize().height;
    const unsigned capped_height =
      std::max(minimum_height, maximum_layout_height);

    if (size.height == 0)
      size.height = capped_height;
    else
      size.height = std::clamp(size.height, minimum_height, capped_height);
  }

  return size;
}

void
VScrollWidget::Initialise(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  assert(!visible);

  WindowStyle style;
  style.ControlParent();
  style.Hide();

  /* Do not TabStop the panel itself.  With TabStop, dialog
     FocusNext/Previous land on the panel and Up/Down scroll it
     (including past the real content).  Child tab-stops such as
     RichTextWindow are found via ControlParent instead. */

  VScrollPanelListener &listener = *this;
  SetWindow(std::make_unique<VScrollPanel>(parent, look, rc, style,
                                           listener));

  widget->Initialise(GetWindow(), AdjustForScrollbar(rc));
}

void
VScrollWidget::Prepare(ContainerWindow &, const PixelRect &rc) noexcept
{
  assert(!visible);

  GetWindow().Move(rc);

  widget->Prepare(GetWindow(), AdjustForScrollbar(rc));
}

bool
VScrollWidget::Save(bool &changed) noexcept
{
  return widget->Save(changed);
}

bool
VScrollWidget::Click() noexcept
{
  return widget->Click();
}

void
VScrollWidget::ReClick() noexcept
{
  widget->ReClick();
}

void
VScrollWidget::Show(const PixelRect &rc) noexcept
{
  WindowWidget::Show(rc);

  UpdateVirtualHeight(rc);

  visible = true;

  if (reserve_scrollbar) {
    /* Viewport-sized child: paint uses VScrollPanel::GetOrigin().
       Avoids Move()/Invalidate of a full virtual-height window on
       every smooth-scroll tick. */
    widget->Show(GetWindow().GetPhysicalRect());
    UpdateVirtualHeight(rc);
    widget->Move(GetWindow().GetPhysicalRect());
  } else {
    widget->Show(GetWindow().GetVirtualRect());

    /* Remeasure after the child has laid out.  Forms may change
       row sizes during Show(). */
    UpdateVirtualHeight(rc);
    widget->Move(GetWindow().GetVirtualRect());
  }
}

bool
VScrollWidget::Leave() noexcept
{
  return widget->Leave();
}

void
VScrollWidget::Hide() noexcept
{
  WindowWidget::Hide();

  visible = false;
  widget->Hide();
}

void
VScrollWidget::Move(const PixelRect &rc) noexcept
{
  /* Match Prepare(): the scroll panel may exist but not be shown yet
     (PagerWidget::Move during layout before Show, or after Hide). */
  if (visible)
    WindowWidget::Move(rc);
  else if (IsDefined())
    GetWindow().Move(rc);

  /* Update virtual height when moved (e.g., when expert mode toggles
     and child widget changes size) */
  if (visible) {
    UpdateVirtualHeight(rc);
    widget->Move(reserve_scrollbar
                 ? GetWindow().GetPhysicalRect()
                 : GetWindow().GetVirtualRect());
  }
}

bool
VScrollWidget::SetFocus() noexcept
{
  if (reserve_scrollbar) {
    /* Try to give focus to the content widget first (for
       link/checkbox navigation in rich text). */
    if (widget->SetFocus())
      return true;

    /* Fall back to the scroll panel itself. */
    GetWindow().SetFocus();
    return true;
  }

  return widget->SetFocus();
}

bool
VScrollWidget::HasFocus() const noexcept
{
  return widget->HasFocus();
}

bool
VScrollWidget::KeyPress(unsigned key_code) noexcept
{
  /* Let the child widget handle the key first
     (for link/checkbox navigation in rich text). */
  if (widget->KeyPress(key_code))
    return true;

  if (!reserve_scrollbar)
    return false;

  /* Handle scrolling keys — only consume directional keys if there
     is room to scroll.  Otherwise return false so the parent widget
     (e.g. QuickGuidePageWidget) can move focus to other controls.
     PageUp/PageDown/Home/End are always consumed when a scrollbar
     is present. */
  const int step = GetWindow().GetScrollStep();
  const int page = std::max(1,
    static_cast<int>(GetWindow().GetSize().height) - step);

  switch (key_code) {
  case KEY_UP:
    if (GetWindow().CanScrollUp()) {
      GetWindow().ScrollBy(-step);
      return true;
    }
    return false;

  case KEY_DOWN:
    if (GetWindow().CanScrollDown()) {
      GetWindow().ScrollBy(step);
      return true;
    }
    return false;

  case KEY_PRIOR: // Page Up
    if (GetWindow().CanScrollUp()) {
      GetWindow().ScrollBy(-page);
      return true;
    }
    return false;

  case KEY_NEXT: // Page Down
    if (GetWindow().CanScrollDown()) {
      GetWindow().ScrollBy(page);
      return true;
    }
    return false;

  case KEY_HOME:
    if (GetWindow().CanScrollUp()) {
      GetWindow().ScrollBy(-static_cast<int>(
        GetWindow().GetSize().height * 100));
      return true;
    }
    return false;

  case KEY_END:
    if (GetWindow().CanScrollDown()) {
      GetWindow().ScrollBy(static_cast<int>(
        GetWindow().GetSize().height * 100));
      return true;
    }
    return false;

  default:
    return false;
  }
}

void
VScrollWidget::OnVScrollPanelChange() noexcept
{
  if (!visible)
    return;

  if (reserve_scrollbar) {
    /* Origin-only updates already Invalidate() the panel.  Do not
       Move() the child (Window::Move always invalidates, and the
       child stays at the physical viewport).  Resize still goes
       through VScrollWidget::Move. */
    return;
  }

  UpdateVirtualHeight(GetWindow().GetClientRect());
  widget->Move(GetWindow().GetVirtualRect());
}

bool
VScrollWidget::OnVScrollPanelGesture(const char *gesture) noexcept
{
  if (!gesture_callback)
    return false;

  if (StringIsEqual(gesture, "R")) {
    /* Swipe right = next page (+1) */
    gesture_callback(true);
    return true;
  }

  if (StringIsEqual(gesture, "L")) {
    /* Swipe left = previous page (-1) */
    gesture_callback(false);
    return true;
  }

  return false;
}
