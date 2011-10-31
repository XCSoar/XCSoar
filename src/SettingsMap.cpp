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

#include "SettingsMap.hpp"

void
SETTINGS_MAP::SetDefaults()
{
  circle_zoom_enabled = true;
  max_auto_zoom_distance = fixed(10000); /* 100 km */
  topography_enabled = true;
  terrain.SetDefaults();
  aircraft_symbol = acSimple;
  trail_drift_enabled = true;
  detour_cost_markers_enabled = false;
  display_track_bearing = dtbAuto;
  auto_zoom_enabled = false;
  snail_scaling_enabled = true;
  snail_type = stStandardVario;
  wind_arrow_style = 0;
  waypoint.SetDefaults();
  trail_length = TRAIL_LONG;
  airspace.SetDefaults();
  glider_screen_position = 20; // 20% from bottom
  circling_orientation = TRACKUP;
  cruise_orientation = TRACKUP;
  show_flarm_on_map = true;
  show_thermal_profile = true;
}
