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

#include "AutoQNH.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "SettingsComputer.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

#define QNH_TIME 10

namespace AutoQNH
{
  unsigned countdown_autoqnh = QNH_TIME;
}

void
AutoQNH::Process(const NMEA_INFO &basic, DERIVED_INFO &calculated,
        const SETTINGS_COMPUTER &settings_computer, const Waypoints &way_points)
{
  if (!calculated.flight.OnGround // must be on ground
      || IsFinished()    // only do it once
      || !basic.gps.real // never in replay mode / simulator
      || !basic.LocationAvailable // Reject if no valid GPS fix
      || !basic.PressureAltitudeAvailable // Reject if no pressure altitude
      || settings_computer.pressure_available // Reject if QNH already known
    ) {
    if (!IsFinished())
      Reset(); // restart if havent performed

    return;
  }

  if (!IsFinished())
    countdown_autoqnh--;

  if (!countdown_autoqnh) {
    if (CalculateQNH(basic, calculated, settings_computer.pressure, way_points))
      countdown_autoqnh = UINT_MAX; // disable after performing once
    else
      Reset();
  }
}

void
AutoQNH::Reset()
{
  countdown_autoqnh = QNH_TIME;
}

bool
AutoQNH::IsFinished()
{
  return countdown_autoqnh > QNH_TIME;
}

bool
AutoQNH::CalculateQNH(const NMEA_INFO &basic, DERIVED_INFO &calculated,
                      const AtmosphericPressure &pressure,
                      const Waypoints &way_points)
{
  const Waypoint *next_wp;
  next_wp = way_points.lookup_location(basic.Location, fixed(1000));

  if (next_wp && next_wp->IsAirport())
    CalculateQNH(basic, calculated, pressure, next_wp->Altitude);
  else if (calculated.TerrainValid)
    CalculateQNH(basic, calculated, pressure, calculated.TerrainAlt);
  else
    return false;

  return true;
}

void
AutoQNH::CalculateQNH(const NMEA_INFO &basic, DERIVED_INFO &calculated,
                      const AtmosphericPressure &pressure,
                      fixed altitude)
{
  calculated.pressure.set_QNH(pressure.FindQNHFromPressureAltitude(basic.PressureAltitude, altitude));
  calculated.pressure_available.Update(basic.Time);
}
