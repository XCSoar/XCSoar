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

#include "Settings.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "OS/Clock.hpp"

void
PolarSettings::SetDefaults()
{
  degradation_factor = 1;
  bugs = 1;
  glide_polar_task = GlidePolar::Invalid();
  ballast_timer_active = false;
  auto_bugs = false;
}

void
PlacesOfInterestSettings::ClearHome()
{
  home_waypoint = -1;
  home_location_available = false;
}

void
PlacesOfInterestSettings::SetHome(const Waypoint &wp)
{
  home_waypoint = wp.id;
  home_location = wp.location;
  home_location_available = true;
}

void
FeaturesSettings::SetDefaults()
{
  final_glide_terrain = FinalGlideTerrain::TERRAIN_LINE;
  block_stf_enabled = false;
  nav_baro_altitude_enabled = true;
}

void
ComputerSettings::SetDefaults()
{
  wind.SetDefaults();
  polar.SetDefaults();
  team_code.SetDefaults();
  poi.SetDefaults();
  features.SetDefaults();
  circling.SetDefaults();
  wave.SetDefaults();

  average_eff_time = ae30seconds;
  set_system_time_from_gps = false;
  utc_offset = RoughTimeDelta::FromSeconds(GetSystemUTCOffset());
  forecast_temperature = Temperature::FromCelsius(25);
  pressure = AtmosphericPressure::Standard();
  pressure_available.Clear();
  airspace.SetDefaults();
  task.SetDefaults();
  contest.SetDefaults();
  logger.SetDefaults();

#ifdef HAVE_TRACKING
  tracking.SetDefaults();
#endif
  weather.SetDefaults();
}
