// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Derived.hpp"

void
TerrainInfo::Clear()
{
  terrain_valid = false;

  terrain_base_valid = false;

  altitude_agl_valid = false;
  altitude_agl = 0;

  terrain_warning_location.SetInvalid();
}

void
TeamInfo::Clear()
{
  teammate_available = false;

  own_teammate_code.Clear();

  flarm_teammate_code.Clear();
}

void
AirspaceWarningsInfo::Clear()
{
  latest.Clear();
}

void
DerivedInfo::Reset()
{
  date_time_local = BrokenDateTime::Invalid();

  VarioInfo::Clear();
  ClimbInfo::Clear();
  CirclingInfo::Clear();
  TeamInfo::Clear();
  TerrainInfo::Clear();
  TeamInfo::Clear();

  pressure_available.Clear();

  climb_history.Clear();

  wave.Clear();

  estimated_wind_available.Clear();
  wind_available.Clear();
  wind_source = WindSource::NONE;
  head_wind_available.Clear();
  sun_data_available.Clear();
  task_stats.reset();
  ordered_task_stats.reset();
  common_stats.Reset();
  contest_stats.Reset();

  flight.Reset();

  thermal_encounter_band.Reset();
  thermal_encounter_collection.Reset();

  thermal_locator.Clear();

  trace_history.clear();

  auto_mac_cready_available.Clear();

  glide_polar_safety = GlidePolar::Invalid();

  airspace_warnings.Clear();

  planned_route.clear();
}

void
DerivedInfo::Expire(TimeStamp Time) noexcept
{
  // NOTE: wind_available is deliberately not expired. Expiry happens automatically
  // due to the expiration of the real wind source. If wind_available would be
  // expired here (with an shorter expiry time then the source) this would lead
  // to alternating valid/invalid transitions (valid after the source is copied,
  // Invalidated shortly after the copy here).

  /* the estimated wind remains valid for an hour */
  estimated_wind_available.Expire(Time, std::chrono::hours(1));

  head_wind_available.Expire(Time, std::chrono::seconds(3));

  auto_mac_cready_available.Expire(Time, std::chrono::hours(1));
  sun_data_available.Expire(Time, std::chrono::hours(1));
}


double
DerivedInfo::CalculateWorkingFraction(const double h, const double safety_height) const
{
  const double h_floor = GetTerrainBaseFallback() + safety_height;
  const double h_band = (common_stats.height_max_working - h_floor);
  if (h_band>0) {
    const double h_actual = h - h_floor;
    return h_actual / h_band;
  } else
    return 1;
}
