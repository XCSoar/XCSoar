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

#include "Widget.hpp"
#include "Screen/Point.hpp"

Widget::~Widget()
{
}

NullWidget::~NullWidget()
{
}

PixelSize
NullWidget::GetMinimumSize() const
{
  return PixelSize{0,0};
}

PixelSize
NullWidget::GetMaximumSize() const
{
  /* map to GetMinimumSize() by default, so simple derived classes
     need to implement only GetMinimumSize() */
  return GetMinimumSize();
}

void
NullWidget::Initialise(ContainerWindow &parent, const PixelRect &rc)
{
}

void
NullWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
}

void
NullWidget::Unprepare()
{
}

bool
NullWidget::Save(bool &changed)
{
  return true;
}

bool
NullWidget::Click()
{
  return true;
}

void
NullWidget::ReClick()
{
}

bool
NullWidget::Leave()
{
  return true;
}

void
NullWidget::Move(const PixelRect &rc)
{
  Hide();
  Show(rc);
}

bool
NullWidget::SetFocus()
{
  return false;
}

bool
NullWidget::KeyPress(unsigned key_code)
{
  return false;
}
