/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Screen/SolidContainerWindow.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"

SolidWidget::~SolidWidget()
{
  delete widget;
  DeleteWindow();
}

PixelSize
SolidWidget::GetMinimumSize() const
{
  return widget->GetMinimumSize();
}

PixelSize
SolidWidget::GetMaximumSize() const
{
  return widget->GetMaximumSize();
}

constexpr
static PixelRect
ToOrigin(PixelRect rc)
{
  return PixelRect(PixelPoint(0, 0), rc.GetSize());
}

void
SolidWidget::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
  WindowStyle style;
  style.ControlParent();
  style.Hide();

  auto window = new SolidContainerWindow();
  window->Create(parent, rc, UIGlobals::GetDialogLook().background_color,
                 style);
  SetWindow(window);

  widget->Initialise(*window, ToOrigin(rc));
}

void
SolidWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  widget->Prepare((ContainerWindow &)GetWindow(), ToOrigin(rc));
}

void
SolidWidget::Unprepare()
{
  widget->Unprepare();
}

bool
SolidWidget::Save(bool &changed)
{
  return widget->Save(changed);
}

bool
SolidWidget::Click()
{
  return widget->Click();
}

void
SolidWidget::ReClick()
{
  widget->ReClick();
}

void
SolidWidget::Show(const PixelRect &rc)
{
  widget->Show(ToOrigin(rc));

  WindowWidget::Show(rc);
}

bool
SolidWidget::Leave()
{
  return widget->Leave();
}

void
SolidWidget::Hide()
{
  WindowWidget::Hide();
  widget->Hide();
}

void
SolidWidget::Move(const PixelRect &rc)
{
  WindowWidget::Move(rc);
  widget->Move(ToOrigin(rc));
}

bool
SolidWidget::SetFocus()
{
  return widget->SetFocus();
}

bool
SolidWidget::KeyPress(unsigned key_code)
{
  return widget->KeyPress(key_code);
}
