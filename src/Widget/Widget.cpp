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

#include "Widget.hpp"
#include "ui/dim/Size.hpp"

PixelSize
NullWidget::GetMinimumSize() const noexcept
{
  return PixelSize{0,0};
}

PixelSize
NullWidget::GetMaximumSize() const noexcept
{
  /* map to GetMinimumSize() by default, so simple derived classes
     need to implement only GetMinimumSize() */
  return GetMinimumSize();
}

void
NullWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept
{
}

void
NullWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
}

void
NullWidget::Unprepare() noexcept
{
}

bool
NullWidget::Save(bool &changed) noexcept
{
  return true;
}

bool
NullWidget::Click() noexcept
{
  return true;
}

void
NullWidget::ReClick() noexcept
{
}

bool
NullWidget::Leave() noexcept
{
  return true;
}

void
NullWidget::Move(const PixelRect &rc) noexcept
{
  Hide();
  Show(rc);
}

bool
NullWidget::SetFocus() noexcept
{
  return false;
}

bool
NullWidget::KeyPress(unsigned key_code) noexcept
{
  return false;
}
