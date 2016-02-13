/* Copyright_License {

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

#include "AirspaceAltitude.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Navigation/Aircraft.hpp"

void
AirspaceAltitude::SetFlightLevel(const AtmosphericPressure &press)
{
  static constexpr double fl_feet_to_m(30.48);
  if (reference == AltitudeReference::STD)
    altitude = press.PressureAltitudeToQNHAltitude(flight_level * fl_feet_to_m);
}

void
AirspaceAltitude::SetGroundLevel(const double alt)
{
  if (reference == AltitudeReference::AGL)
    altitude = altitude_above_terrain + alt;
}

bool
AirspaceAltitude::IsAbove(const AltitudeState &state, const double margin) const
{
  return GetAltitude(state) >= state.altitude - margin;
}

bool
AirspaceAltitude::IsBelow(const AltitudeState &state, const double margin) const
{
  return GetAltitude(state) <= state.altitude + margin ||
    /* special case: GND is always "below" the aircraft, even if the
       aircraft's AGL altitude turns out to be negative due to terrain
       file inaccuracies */
    IsTerrain();
}

double
AirspaceAltitude::GetAltitude(const AltitudeState &state) const
{
  // TODO: check if state.altitude_agl is valid
  return reference == AltitudeReference::AGL
    ? altitude_above_terrain + (state.altitude - state.altitude_agl)
    : altitude;
}
