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

#include "ManagedWidget.hpp"
#include "Widget.hpp"

#include <assert.h>

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
  widget = nullptr;
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

#ifndef NDEBUG
  have_position = true;
#endif

  if (widget != nullptr && prepared && visible)
    widget->Move(position);
}

void
ManagedWidget::Prepare()
{
  assert(have_position);

  if (widget == nullptr || prepared)
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

  if (widget == nullptr)
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
  if (widget != nullptr && prepared && visible) {
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

bool
ManagedWidget::SetFocus()
{
  return IsVisible() && widget->SetFocus();
}

bool
ManagedWidget::KeyPress(unsigned key_code)
{
  return IsVisible() && widget->KeyPress(key_code);
}
