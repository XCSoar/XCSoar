/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Form/Tabbed.hpp"

#include <assert.h>

TabbedControl::TabbedControl(ContainerWindow &parent,
                             PixelScalar x, PixelScalar y,
                             UPixelScalar width, UPixelScalar height,
                             const WindowStyle style)
  :current(0)
{
  set(parent, x, y, width, height, style);
}

void
TabbedControl::AddClient(Window *w)
{
  if (current != tabs.size())
    w->hide();

  tabs.append(w);

  const PixelRect rc = get_client_rect();
  w->move(rc.left, rc.top, rc.right, rc.bottom);
}

void
TabbedControl::SetCurrentPage(unsigned i)
{
  assert(i < tabs.size());

  if (i == current)
    return;

  tabs[current]->hide();
  current = i;
  tabs[current]->show();
}

void
TabbedControl::NextPage()
{
  if (tabs.size() < 2)
    return;

  assert(current < tabs.size());

  SetCurrentPage((current + 1) % tabs.size());
}

void
TabbedControl::PreviousPage()
{
  if (tabs.size() < 2)
    return;

  assert(current < tabs.size());

  SetCurrentPage((current + tabs.size() - 1) % tabs.size());
}

bool
TabbedControl::on_resize(UPixelScalar width, UPixelScalar height)
{
  ContainerWindow::on_resize(width, height);

  const PixelRect rc = get_client_rect();
  for (unsigned i = tabs.size(); i--;)
    tabs[i]->move(rc.left, rc.top, rc.right, rc.bottom);

  return true;
}
