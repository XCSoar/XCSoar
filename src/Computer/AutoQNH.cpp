// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

  if (next_wp && next_wp->IsAirport() && next_wp->has_elevation)
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
