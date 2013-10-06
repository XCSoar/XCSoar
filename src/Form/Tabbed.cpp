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

#include "Tabbed.hpp"

#include <assert.h>

TabbedControl::TabbedControl(ContainerWindow &parent, PixelRect rc,
                             const WindowStyle style)
  :page_flipped_callback(NULL)
{
  Create(parent, rc, style);
}

TabbedControl::~TabbedControl()
{
  Destroy();
}

void
TabbedControl::AddPage(Widget *w)
{
  assert(IsDefined());

  pager.Add(w);
}

bool
TabbedControl::SetCurrentPage(unsigned i, bool click)
{
  const unsigned old_current = pager.GetCurrentIndex();

  bool success = pager.SetCurrent(i, click);

  if (success && old_current != pager.GetCurrentIndex() &&
      page_flipped_callback != NULL)
    page_flipped_callback();

  return success;
}

void
TabbedControl::NextPage()
{
  if (pager.Next(true) && page_flipped_callback != NULL)
    page_flipped_callback();
}

void
TabbedControl::PreviousPage()
{
  if (pager.Previous(true) && page_flipped_callback != NULL)
    page_flipped_callback();
}

bool
TabbedControl::ClickPage(unsigned i)
{
  return pager.SetCurrent(i, true);
}

bool
TabbedControl::Save(bool &changed)
{
  return pager.Save(changed);
}

void
TabbedControl::UpdateLayout()
{
  pager.Move(GetClientRect());
}

void
TabbedControl::OnResize(PixelSize new_size)
{
  ContainerWindow::OnResize(new_size);

  UpdateLayout();
}

void
TabbedControl::OnCreate()
{
  ContainerWindow::OnCreate();

  const PixelRect rc = GetClientRect();
  pager.Initialise(*this, rc);
  pager.Prepare(*this, rc);
  pager.Show(rc);
}

void
TabbedControl::OnDestroy()
{
  pager.Hide();
  pager.Unprepare();

  ContainerWindow::OnDestroy();
}
