// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QuickGuideScrollWidget.hpp"
#include "Asset.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"

QuickGuideScrollWidget::QuickGuideScrollWidget(std::unique_ptr<Widget> &&_widget,
                                               const DialogLook &_look) noexcept
  :look(_look),
   widget(std::move(_widget))
{
}

void
QuickGuideScrollWidget::Initialise(ContainerWindow &parent,
                                   const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.ControlParent();
  style.Hide();

  VScrollPanelListener &listener = *this;
  SetWindow(std::make_unique<VScrollPanel>(parent, look, rc, style,
                                           listener));

  widget->Initialise(GetWindow(), ReserveScrollbar(rc));
}

void
QuickGuideScrollWidget::Prepare(ContainerWindow &, const PixelRect &rc) noexcept
{
  GetWindow().Move(rc);
  widget->Prepare(GetWindow(), ReserveScrollbar(rc));
}

void
QuickGuideScrollWidget::Show(const PixelRect &rc) noexcept
{
  WindowWidget::Show(rc);

  UpdateVirtualHeight(rc);
  visible = true;
  widget->Show(ReserveScrollbar(GetWindow().GetVirtualRect()));
  UpdateVirtualHeight(rc);
  widget->Move(ReserveScrollbar(GetWindow().GetVirtualRect()));
}


void
QuickGuideScrollWidget::Hide() noexcept
{
  WindowWidget::Hide();
  visible = false;
  widget->Hide();
}

void
QuickGuideScrollWidget::Move(const PixelRect &rc) noexcept
{
  WindowWidget::Move(rc);

  if (visible) {
    UpdateVirtualHeight(rc);
    widget->Move(ReserveScrollbar(GetWindow().GetVirtualRect()));
  }
}


unsigned
QuickGuideScrollWidget::GetScrollbarWidth() noexcept
{
  return HasPointer()
    ? Layout::GetMinimumControlHeight()
    : Layout::VptScale(10);
}

PixelRect
QuickGuideScrollWidget::ReserveScrollbar(PixelRect rc) const noexcept
{
  const unsigned scrollbar_width = GetScrollbarWidth();
  if (scrollbar_width == 0)
    return rc;

  if (rc.GetWidth() > scrollbar_width)
    rc.right -= scrollbar_width;

  return rc;
}

unsigned
QuickGuideScrollWidget::CalcVirtualHeight(const PixelRect &rc) const noexcept
{
  const unsigned height = rc.GetHeight();
  const unsigned max_height = widget->GetMaximumSize().height;
  return max_height > height ? max_height : height;
}

void
QuickGuideScrollWidget::UpdateVirtualHeight(const PixelRect &rc) noexcept
{
  GetWindow().SetVirtualHeight(CalcVirtualHeight(rc));
}

void
QuickGuideScrollWidget::OnVScrollPanelChange() noexcept
{
  if (visible) {
    UpdateVirtualHeight(GetWindow().GetClientRect());
    widget->Move(ReserveScrollbar(GetWindow().GetVirtualRect()));
  }
}
