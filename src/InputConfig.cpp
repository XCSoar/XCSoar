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

#include "InputConfig.hpp"

#include <algorithm>

void
InputConfig::SetDefaults()
{
  _tcscpy(mode_map[0], _T("default"));
  _tcscpy(mode_map[1], _T("pan"));
  _tcscpy(mode_map[2], _T("infobox"));
  _tcscpy(mode_map[3], _T("Menu"));
  mode_map_count = 4;

  std::fill(&Key2Event[0][0], &Key2Event[MAX_MODE][MAX_KEY], 0);
  std::fill(&GC2Event[0][0], &GC2Event[MAX_MODE][GCE_COUNT], 0);

  /* This is initialized with 1 because event 0 is reserved - it
     stands for "no event" */
  Events_count = 1;
}
