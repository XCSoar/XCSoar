// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VScrollWidget.hpp"
#include "Asset.hpp"
#include "Form/Panel.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/KeyCode.hpp"
#include "util/StringAPI.hxx"

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
    return max_height > height ? max_height : height;
  }

  /* Flexible form widgets: only scroll when the widget truly
     cannot compress to fit the viewport (min_height > height). */
  if (max_height <= height)
    return max_height;

  const unsigned min_height = widget->GetMinimumSize().height;
  return std::max(min_height, height);
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
  return widget->GetMaximumSize();
}

void
VScrollWidget::Initialise(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  assert(!visible);

  WindowStyle style;
  style.ControlParent();
  style.Hide();

  if (reserve_scrollbar)
    style.TabStop();

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
  widget->Show(GetWindow().GetVirtualRect());

  if (reserve_scrollbar) {
    /* Rich-text content may update its maximum size after the
       initial Show (e.g. after text layout).  Re-measure. */
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
  WindowWidget::Move(rc);

  /* Update virtual height when moved (e.g., when expert mode toggles
     and child widget changes size) */
  if (visible) {
    UpdateVirtualHeight(rc);
    widget->Move(GetWindow().GetVirtualRect());
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
  if (visible) {
    UpdateVirtualHeight(GetWindow().GetClientRect());
    widget->Move(GetWindow().GetVirtualRect());
  }
}

bool
VScrollWidget::OnVScrollPanelGesture(const TCHAR *gesture) noexcept
{
  if (!gesture_callback)
    return false;

  if (StringIsEqual(gesture, _T("R"))) {
    /* Swipe right = next page (+1) */
    gesture_callback(true);
    return true;
  }

  if (StringIsEqual(gesture, _T("L"))) {
    /* Swipe left = previous page (-1) */
    gesture_callback(false);
    return true;
  }

  return false;
}
