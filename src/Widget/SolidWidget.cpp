// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SolidWidget.hpp"
#include "ui/window/SolidContainerWindow.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"

SolidWidget::~SolidWidget() noexcept
{
  widget.reset();
  DeleteWindow();
}

PixelSize
SolidWidget::GetMinimumSize() const noexcept
{
  return widget->GetMinimumSize();
}

PixelSize
SolidWidget::GetMaximumSize() const noexcept
{
  return widget->GetMaximumSize();
}

constexpr
static PixelRect
ToOrigin(PixelRect rc) noexcept
{
  return PixelRect(PixelPoint(0, 0), rc.GetSize());
}

void
SolidWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.ControlParent();
  style.Hide();

  auto window = std::make_unique<SolidContainerWindow>();
  window->Create(parent, rc, UIGlobals::GetDialogLook().background_color,
                 style);
  SetWindow(std::move(window));

  widget->Initialise((ContainerWindow &)GetWindow(), ToOrigin(rc));
}

void
SolidWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  widget->Prepare((ContainerWindow &)GetWindow(), ToOrigin(rc));
}

void
SolidWidget::Unprepare() noexcept
{
  widget->Unprepare();
}

bool
SolidWidget::Save(bool &changed) noexcept
{
  return widget->Save(changed);
}

bool
SolidWidget::Click() noexcept
{
  return widget->Click();
}

void
SolidWidget::ReClick() noexcept
{
  widget->ReClick();
}

void
SolidWidget::Show(const PixelRect &rc) noexcept
{
  widget->Show(ToOrigin(rc));

  WindowWidget::Show(rc);
}

bool
SolidWidget::Leave() noexcept
{
  return widget->Leave();
}

void
SolidWidget::Hide() noexcept
{
  WindowWidget::Hide();
  widget->Hide();
}

void
SolidWidget::Move(const PixelRect &rc) noexcept
{
  WindowWidget::Move(rc);
  widget->Move(ToOrigin(rc));
}

bool
SolidWidget::SetFocus() noexcept
{
  return widget->SetFocus();
}

bool
SolidWidget::HasFocus() const noexcept
{
  return widget->HasFocus();
}

bool
SolidWidget::KeyPress(unsigned key_code) noexcept
{
  return widget->KeyPress(key_code);
}
