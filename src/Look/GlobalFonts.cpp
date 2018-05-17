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

#include "GlobalFonts.hpp"
#include "FontSettings.hpp"
#include "Screen/Font.hpp"

/// text names on the map
Font Fonts::map;
/// menu buttons, waypoint selection, messages, etc.
Font Fonts::map_bold;

bool
Fonts::Load(const FontSettings &settings)
{
  map.Load(settings.map);
  map_bold.Load(settings.map_bold);

  return map.IsDefined() && map_bold.IsDefined();
}

void
Fonts::Deinitialize()
{
  map.Destroy();
  map_bold.Destroy();
}
