/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Math/FastMath.h"
#include "Util/StaticString.hpp"

#include <stdio.h>

void
AirspaceAltitude::SetFlightLevel(const AtmosphericPressure &press)
{
  static const fixed fl_feet_to_m(30.48);
  if (type == Type::FL)
    altitude = press.PressureAltitudeToQNHAltitude(flight_level * fl_feet_to_m);
}

void
AirspaceAltitude::SetGroundLevel(const fixed alt)
{
  if (type == Type::AGL)
    altitude = altitude_above_terrain + alt;
}

const tstring
AirspaceAltitude::GetAsText(const bool concise) const
{
  StaticString<64> buffer;

  switch (type) {
  case Type::AGL:
    if (!positive(altitude_above_terrain))
      buffer = _T("GND");
    else
      buffer.Format(_T("%d AGL"), iround(altitude_above_terrain));

    break;
  case Type::FL:
    buffer.Format(_T("FL%d"), iround(flight_level));
    break;
  case Type::MSL:
    buffer.Format(_T("%d"), iround(altitude));
    break;
  case Type::UNDEFINED:
    buffer.clear();
    break;
  }

  if (!concise && type != Type::MSL && positive(altitude))
    buffer.AppendFormat(_T(" %d"), iround(altitude));

  return tstring(buffer);
}

bool
AirspaceAltitude::IsAbove(const AltitudeState &state, const fixed margin) const
{
  return GetAltitude(state) >= state.altitude - margin;
}

bool
AirspaceAltitude::IsBelow(const AltitudeState &state, const fixed margin) const
{
  return GetAltitude(state) <= state.altitude + margin ||
    /* special case: GND is always "below" the aircraft, even if the
       aircraft's AGL altitude turns out to be negative due to terrain
       file inaccuracies */
    (type == Type::AGL && !positive(altitude_above_terrain));
}

fixed
AirspaceAltitude::GetAltitude(const AltitudeState &state) const
{
  return type == Type::AGL
    ? altitude_above_terrain + (state.altitude - state.altitude_agl)
    : altitude;
}
