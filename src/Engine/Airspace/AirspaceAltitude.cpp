// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceAltitude.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Navigation/Aircraft.hpp"

void
AirspaceAltitude::SetFlightLevel(const AtmosphericPressure press) noexcept
{
  static constexpr double fl_feet_to_m(30.48);
  if (reference == AltitudeReference::STD)
    altitude = press.PressureAltitudeToQNHAltitude(flight_level * fl_feet_to_m);
}

void
AirspaceAltitude::SetGroundLevel(const double alt) noexcept
{
  if (reference == AltitudeReference::AGL)
    altitude = altitude_above_terrain + alt;
}

bool
AirspaceAltitude::IsAbove(const AltitudeState &state,
                          const double margin) const noexcept
{
  return GetAltitude(state) >= state.altitude - margin;
}

bool
AirspaceAltitude::IsBelow(const AltitudeState &state,
                          const double margin) const noexcept
{
  return GetAltitude(state) <= state.altitude + margin ||
    /* special case: GND is always "below" the aircraft, even if the
       aircraft's AGL altitude turns out to be negative due to terrain
       file inaccuracies */
    IsTerrain();
}

double
AirspaceAltitude::GetAltitude(const AltitudeState &state) const noexcept
{
  // TODO: check if state.altitude_agl is valid
  return reference == AltitudeReference::AGL
    ? altitude_above_terrain + (state.altitude - state.altitude_agl)
    : altitude;
}
