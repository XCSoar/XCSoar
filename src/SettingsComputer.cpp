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

#include "SettingsComputer.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "OS/Clock.hpp"
#include "Asset.hpp"

void
WindSettings::SetDefaults()
{
  auto_wind_mode = AUTOWIND_CIRCLING;
  use_external_wind = true;
  manual_wind_available.Clear();
}

void
LoggerSettings::SetDefaults()
{
  logger_time_step_cruise = 5;
  logger_time_step_circling = 1;
  logger_short_name = false;
  auto_logger_disabled = false;
}

void
PolarSettings::SetDefaults()
{
  ballast_timer_active = false;
}

void
SoundSettings::SetDefaults()
{
  sound_vario_enabled = true;
  sound_task_enabled = true;
  sound_modes_enabled = true;
  sound_volume = 80;
  sound_deadband = 5;
}

void
TeamCodeSettings::SetDefaults()
{
  team_code_reference_waypoint = -1;
  team_flarm_tracking = false;
  team_code_valid = false;
  team_flarm_callsign.clear();
  team_flarm_id.Clear();
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
  final_glide_terrain = FGT_LINE;
  block_stf_enabled = false;
  nav_baro_altitude_enabled = true;
}

void
SETTINGS_COMPUTER::SetDefaults()
{
  WindSettings::SetDefaults();
  LoggerSettings::SetDefaults();
  PolarSettings::SetDefaults();
  SoundSettings::SetDefaults();
  TeamCodeSettings::SetDefaults();
  VoiceSettings::SetDefaults();
  PlacesOfInterestSettings::SetDefaults();
  FeaturesSettings::SetDefaults();
  PlacesOfInterestSettings::SetDefaults();

  external_trigger_cruise_enabled =false;
  average_eff_time = ae30seconds;
  set_system_time_from_gps = is_altair() && is_embedded();
  utc_offset = GetSystemUTCOffset();
  forecast_temperature = fixed(25);
  pressure = AtmosphericPressure::Standard();
  pressure_available.Clear();
  airspace.SetDefaults();
  task.SetDefaults();

#ifdef HAVE_TRACKING
  tracking.SetDefaults();
#endif
}
