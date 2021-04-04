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

#include "ManagedWidget.hpp"
#include "Widget.hpp"

#include <cassert>

void
ManagedWidget::Unprepare() noexcept
{
  Hide();

  if (!IsPrepared())
    return;

  state = State::INITIALISED;
  widget->Unprepare();
}

void
ManagedWidget::Clear() noexcept
{
  Unprepare();

  delete widget;
  widget = nullptr;
}

void
ManagedWidget::Set(Widget *_widget) noexcept
{
  Clear();

  widget = _widget;
  state = State::NONE;
}

void
ManagedWidget::Set(std::unique_ptr<Widget> _widget) noexcept
{
  Set(_widget.release());
}

void
ManagedWidget::Move(const PixelRect &_position) noexcept
{
  position = _position;

#ifndef NDEBUG
  have_position = true;
#endif

  if (IsVisible())
    widget->Move(position);
}

void
ManagedWidget::Initialise(ContainerWindow &_parent, const PixelRect &_position)
{
  assert(parent == nullptr);
  assert(widget == nullptr || state == State::NONE);

  parent = &_parent;
  position = _position;

#ifndef NDEBUG
  have_position = true;
#endif

  if (widget != nullptr) {
    widget->Initialise(*parent, position);
    state = State::INITIALISED;
  }
}

void
ManagedWidget::Prepare()
{
  assert(parent != nullptr);
  assert(have_position);

  if (widget == nullptr)
    return;

  if (state < State::INITIALISED) {
    state = State::INITIALISED;
    widget->Initialise(*parent, position);
  }

  if (state < State::PREPARED) {
    state = State::PREPARED;
    widget->Prepare(*parent, position);
  }
}

void
ManagedWidget::Show() noexcept
{
  assert(have_position);

  if (widget == nullptr)
    return;

  Prepare();

  if (state < State::VISIBLE) {
    state = State::VISIBLE;
    widget->Show(position);
  }
}

void
ManagedWidget::Hide() noexcept
{
  if (IsVisible()) {
    widget->Leave();
    state = State::PREPARED;
    widget->Hide();
  }
}

void
ManagedWidget::SetVisible(bool _visible) noexcept
{
  if (!IsPrepared())
    return;

  if (_visible)
    Show();
  else
    Hide();
}

bool
ManagedWidget::Save(bool &changed)
{
  return !IsPrepared() || widget->Save(changed);
}

bool
ManagedWidget::SetFocus() noexcept
{
  return IsVisible() && widget->SetFocus();
}

bool
ManagedWidget::KeyPress(unsigned key_code) noexcept
{
  return IsVisible() && widget->KeyPress(key_code);
}
