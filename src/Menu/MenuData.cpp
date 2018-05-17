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

#include "MenuData.hpp"

#include <assert.h>

void
Menu::Clear()
{
  for (auto i = items, end = items + MAX_ITEMS; i != end; ++i)
    i->Clear();
}

void
Menu::Add(const TCHAR *label, int location, unsigned event_id)
{
  assert(location >= 0);

  if (location >= MAX_ITEMS)
    return;

  MenuItem &item = items[location];

  item.label = label;
  item.event = event_id;
}

int
Menu::FindByEvent(unsigned event) const
{
  for (unsigned i = 0; i < MAX_ITEMS; ++i)
    if (items[i].event == event)
      return i;

  return -1;
}
