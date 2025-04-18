// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "time/Zone.hxx"

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
  home_elevation_available = false;
}

void
PlacesOfInterestSettings::SetHome(const Waypoint &wp)
{
  home_waypoint = wp.id;
  home_location = wp.location;
  home_location_available = true;
  if (wp.has_elevation) {
    home_elevation = wp.elevation;
    home_elevation_available = true;
  } else
    home_elevation_available = false;
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
  utc_offset = RoughTimeDelta::FromSeconds(GetTimeZoneOffset());
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
  radio.SetDefaults();
  transponder.SetDefaults();
  weglide.SetDefaults();
}
