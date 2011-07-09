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

#include "DisplayMode.hpp"
#include "SettingsMap.hpp"
#include "NMEA/Derived.hpp"

DisplayMode
GetNewDisplayMode(const SETTINGS_MAP &settings_map,
                  const DERIVED_INFO &derived_info)
{
  if (settings_map.UserForceDisplayMode != DM_NONE)
    return settings_map.UserForceDisplayMode;
  else if (derived_info.Circling)
    return DM_CIRCLING;
  else if (derived_info.task_stats.flight_mode_final_glide)
    return DM_FINAL_GLIDE;
  else
    return DM_CRUISE;
}
