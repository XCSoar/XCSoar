/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "ManagedWidget.hpp"
#include "Widget.hpp"

void
ManagedWidget::Unprepare()
{
  Hide();

  if (!IsPrepared())
    return;

  prepared = false;
  widget->Unprepare();
}

void
ManagedWidget::Clear()
{
  Unprepare();

  delete widget;
  widget = NULL;
}

void
ManagedWidget::Set(Widget *_widget)
{
  Clear();

  widget = _widget;
  prepared = false;
}

void
ManagedWidget::Move(const PixelRect &_position)
{
  position = _position;
  have_position = true;

  if (widget != NULL && prepared && visible)
    widget->Move(position);
}

void
ManagedWidget::Prepare()
{
  assert(have_position);

  if (widget == NULL || prepared)
    return;

  widget->Initialise(parent, position);
  widget->Prepare(parent, position);
  prepared = true;
  visible = false;
}

void
ManagedWidget::Show()
{
  assert(have_position);

  if (widget == NULL)
    return;

  Prepare();

  if (!visible) {
    visible = true;
    widget->Show(position);
  }
}

void
ManagedWidget::Hide()
{
  if (widget != NULL && prepared && visible) {
    widget->Leave();
    visible = false;
    widget->Hide();
  }
}

void
ManagedWidget::SetVisible(bool _visible)
{
  if (!IsPrepared())
    return;

  if (_visible)
    Show();
  else
    Hide();
}
