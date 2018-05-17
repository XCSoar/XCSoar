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
DerivedInfo::Expire(double Time)
{
  // NOTE: wind_available is deliberately not expired. Expiry happens automatically
  // due to the expiration of the real wind source. If wind_available would be
  // expired here (with an shorter expiry time then the source) this would lead
  // to alternating valid/invalid transitions (valid after the source is copied,
  // Invalidated shortly after the copy here).

  /* the estimated wind remains valid for an hour */
  estimated_wind_available.Expire(Time, 3600);

  head_wind_available.Expire(Time, 3);

  auto_mac_cready_available.Expire(Time, 3600);
  sun_data_available.Expire(Time, 3600);
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
