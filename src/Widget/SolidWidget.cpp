/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
SolidWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
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
