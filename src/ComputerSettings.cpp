/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "ComputerSettings.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "OS/Clock.hpp"
#include "Asset.hpp"
#include "Atmosphere/Temperature.hpp"

void
WindSettings::SetDefaults()
{
  circling_wind = true;
  zig_zag_wind = true;
  use_external_wind = true;
  manual_wind_available.Clear();
}

void
PolarSettings::SetDefaults()
{
  degradation_factor = fixed_one;
  bugs = fixed_one;
  glide_polar_task = GlidePolar::Invalid();
  ballast_timer_active = false;
}

void
VoiceSettings::SetDefaults()
{
  voice_climb_rate_enabled = false;
  voice_terrain_enabled = false;
  voice_waypoint_distance_enabled = false;
  voice_task_altitude_difference_enabled = false;
  voice_mac_cready_enabled = false;
  voice_new_waypoint_enabled = false;
  voice_in_sector_enabled = false;
  voice_airspace_enabled = false;
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
  final_glide_terrain = FinalGlideTerrain::LINE;
  block_stf_enabled = false;
  nav_baro_altitude_enabled = true;
}

void
ComputerSettings::SetDefaults()
{
  wind.SetDefaults();
  polar.SetDefaults();
  team_code.SetDefaults();
  voice.SetDefaults();
  poi.SetDefaults();
  features.SetDefaults();
  circling.SetDefaults();

  average_eff_time = ae30seconds;
  set_system_time_from_gps = IsAltair() && IsEmbedded();
  utc_offset = GetSystemUTCOffset();
  forecast_temperature = CelsiusToKelvin(fixed(25));
  pressure = AtmosphericPressure::Standard();
  pressure_available.Clear();
  airspace.SetDefaults();
  task.SetDefaults();
  contest.SetDefaults();
  logger.SetDefaults();

#ifdef HAVE_TRACKING
  tracking.SetDefaults();
#endif
}
