// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Settings.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "time/SystemTimeZone.hpp"
#include "time/TimeZones.hpp"

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

#ifdef KOBO
  /* the Kobo has no time zone configuration at all: its clock runs in
     UTC, so the automatic source could only ever yield UTC */
  local_time_source = LocalTimeSource::TIME_ZONE;
#else
  local_time_source = LocalTimeSource::AUTOMATIC;
#endif

  time_zone = "UTC";

  /* #local_time_source was just set to one of the two automatic ones,
     so this does not read #utc_offset back */
  utc_offset = GetCurrentUTCOffset();
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

RoughTimeDelta
ComputerSettings::GetCurrentUTCOffset() const noexcept
{
  switch (local_time_source) {
  case LocalTimeSource::AUTOMATIC:
    return RoughTimeDelta::FromSeconds(GetCurrentTimeZoneOffset());

  case LocalTimeSource::TIME_ZONE:
    if (const auto offset = FindTimeZoneOffset(time_zone.c_str(),
                                               std::chrono::system_clock::now()))
      return RoughTimeDelta::FromSeconds(offset->count());

    /* a time zone which is not in our table (e.g. because it was
       removed from the zoneinfo database): fall back to UTC */
    return RoughTimeDelta::FromSeconds(0);

  case LocalTimeSource::MANUAL_UTC_OFFSET:
    break;
  }

  return utc_offset;
}
