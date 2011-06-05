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

#include "NMEA/Derived.hpp"

void
TERRAIN_ALT_INFO::Clear()
{
  TerrainWarning = false;

  AltitudeAGLValid = false;
  AltitudeAGL = fixed_zero;
}

void
TEAMCODE_INFO::Clear()
{
  teammate_available = false;

  flarm_teammate_code_available = false;
}

void
DERIVED_INFO::reset()
{
  VARIO_INFO::Clear();
  CLIMB_INFO::Clear();
  CIRCLING_INFO::Clear();
  TEAMCODE_INFO::Clear();
  ClimbHistory::Clear();
  TERRAIN_ALT_INFO::Clear();
  TEAMCODE_INFO::Clear();

  Heading = Angle::native(fixed_zero);
  pressure_available.Clear();
  AirspeedAvailable.Clear();
  estimated_wind_available.Clear();
  wind_available.Clear();
  task_stats.reset();
  common_stats.reset();

  flight.flying_state_reset();
  thermal_band.clear();
  thermal_locator.Clear();

  trace_history.clear();

  auto_mac_cready_available.Clear();
}

void
DERIVED_INFO::expire(fixed Time)
{
  /* the estimated wind remains valid for an hour */
  estimated_wind_available.Expire(Time, fixed(3600));
  wind_available.Expire(Time, fixed(600));
  /* the calculated airspeed expires after 5 seconds */
  AirspeedAvailable.Expire(Time, fixed(5));

  auto_mac_cready_available.Expire(Time, fixed(300));
}
