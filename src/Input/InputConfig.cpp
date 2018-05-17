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

#include "InputConfig.hpp"
#include "Util/Macros.hpp"

#include <algorithm>

void
InputConfig::SetDefaults()
{
  modes.resize(4);
  modes[0] = _T("default");
  modes[1] = _T("pan");
  modes[2] = _T("infobox");
  modes[3] = _T("Menu");

  std::fill_n(&Key2Event[0][0], MAX_MODE*MAX_KEY, 0);
#ifdef ENABLE_SDL
  std::fill_n(&Key2EventNonChar[0][0], MAX_MODE*MAX_KEY, 0);
#endif
#ifdef USE_X11
  std::fill_n(&Key2EventFF00[0][0], MAX_MODE * MAX_KEY, 0);
#endif

  Gesture2Event.Clear();

  std::fill_n(&GC2Event[0], ARRAY_SIZE(GC2Event), 0);
  std::fill_n(&N2Event[0], ARRAY_SIZE(N2Event), 0);

  /* This is initialized with 1 because event 0 is reserved - it
     stands for "no event" */
  events.resize(1);

  for (auto i = menus, end = menus + MAX_MODE; i != end; ++i)
    i->Clear();
}
