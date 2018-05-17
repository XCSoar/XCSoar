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

#include "AutoQNH.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Settings.hpp"
#include "Engine/Waypoint/Waypoints.hpp"

void
AutoQNH::Process(const NMEAInfo &basic, DerivedInfo &calculated,
        const ComputerSettings &settings_computer, const Waypoints &way_points)
{
  if (!calculated.flight.on_ground // must be on ground
      || IsFinished()    // only do it once
      || !basic.location_available // Reject if no valid GPS fix
      || !basic.static_pressure_available // Reject if no pressure
      || settings_computer.pressure_available // Reject if QNH already known
    ) {
    if (!IsFinished())
      Reset(); // restart if havent performed

    return;
  }

  if (!IsFinished())
    countdown_autoqnh--;

  if (!countdown_autoqnh) {
    if (CalculateQNH(basic, calculated, way_points))
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

inline bool
AutoQNH::CalculateQNH(const NMEAInfo &basic, DerivedInfo &calculated,
                      const Waypoints &way_points)
{
  const auto next_wp = way_points.LookupLocation(basic.location, 1000);

  if (next_wp && next_wp->IsAirport())
    CalculateQNH(basic, calculated, next_wp->elevation);
  else if (calculated.terrain_valid)
    CalculateQNH(basic, calculated, calculated.terrain_altitude);
  else
    return false;

  return true;
}

inline void
AutoQNH::CalculateQNH(const NMEAInfo &basic, DerivedInfo &calculated,
                      double altitude)
{
  calculated.pressure = AtmosphericPressure::FindQNHFromPressure(basic.static_pressure, altitude);
  calculated.pressure_available.Update(basic.clock);
}
