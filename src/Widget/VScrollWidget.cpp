// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VScrollWidget.hpp"
#include "Form/Panel.hpp"

#include <cassert>

inline unsigned
VScrollWidget::CalcVirtualHeight(const PixelRect &rc) const noexcept
{
  const unsigned height = rc.GetHeight();
  const unsigned max_height = widget->GetMaximumSize().height;
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
VScrollWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  assert(!visible);

  WindowStyle style;
  style.ControlParent();
  style.Hide();

  VScrollPanelListener &listener = *this;
  SetWindow(std::make_unique<VScrollPanel>(parent, look, rc, style,
                                           listener));

  widget->Initialise(GetWindow(), rc);
}

void
VScrollWidget::Prepare(ContainerWindow &, const PixelRect &rc) noexcept
{
  assert(!visible);

  GetWindow().Move(rc);

  widget->Prepare(GetWindow(), rc);
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

bool
VScrollWidget::SetFocus() noexcept
{
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
  return widget->KeyPress(key_code);
}

void
VScrollWidget::OnVScrollPanelChange() noexcept
{
  if (visible) {
    UpdateVirtualHeight(GetWindow().GetClientRect());
    widget->Move(GetWindow().GetVirtualRect());
  }
}
