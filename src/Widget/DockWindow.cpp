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

#include "DockWindow.hpp"
#include "Widget.hpp"

void
DockWindow::SetWidget(Widget *_widget)
{
  assert(IsDefined());
  assert(widget == nullptr);

  widget = _widget;

  if (widget != nullptr) {
    const PixelRect rc = GetClientRect();
    widget->Initialise(*this, rc);
    widget->Prepare(*this, rc);
    widget->Show(rc);
  }
}

void
DockWindow::UnprepareWidget()
{
  assert(widget != nullptr);

  widget->Hide();
  widget->Unprepare();
}

void
DockWindow::DeleteWidget()
{
  if (widget == nullptr)
    return;

  UnprepareWidget();
  delete widget;
  widget = nullptr;
}

void
DockWindow::MoveWidget()
{
  assert(widget != nullptr);

  widget->Move(GetClientRect());
}

bool
DockWindow::SaveWidget(bool &changed)
{
  return widget == nullptr || widget->Save(changed);
}

void
DockWindow::OnResize(PixelSize new_size)
{
  ContainerWindow::OnResize(new_size);

  if (widget != nullptr)
    widget->Move(GetClientRect());
}
