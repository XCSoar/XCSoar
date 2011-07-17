/* Copyright_License {

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
#include "AirspaceAltitude.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Navigation/Aircraft.hpp"
#include "FastMath.h"
#include <stdio.h>

void 
AIRSPACE_ALT::set_flight_level(const AtmosphericPressure &press)
{
  static const fixed fl_feet_to_m(30.48);
  if (Base == abFL)
    Altitude = press.PressureAltitudeToQNHAltitude(FL * fl_feet_to_m);
}

void 
AIRSPACE_ALT::set_ground_level(const fixed alt)
{
  if (Base == abAGL)
    Altitude = AGL+alt;
}

const tstring 
AIRSPACE_ALT::get_as_text(const bool concise) const
{
  TCHAR buffer[64];

  switch (Base) {
  case abAGL:
    if (!positive(AGL)) {
      _tcscpy(buffer, _T("GND"));
    } else {
      _stprintf(buffer, _T("%d AGL"), iround(AGL));
    }
    break;
  case abFL:
    _stprintf(buffer, _T("FL%d"), iround(FL));
    break;
  case abMSL:
    _stprintf(buffer, _T("%d"), iround(Altitude));
    break;
  case abUndef:
  default:
    buffer[0] = _T('\0');
    break;
  };
  if (!concise && Base!=abMSL && positive(Altitude)) {
    TCHAR second[64];
    _stprintf(second, _T(" %d"), iround(Altitude));
    return tstring(buffer) + second;
  } else
    return tstring(buffer);
}


bool AIRSPACE_ALT::is_above  (const ALTITUDE_STATE& state,
                              const fixed margin) const {
  return get_altitude(state) >= state.NavAltitude - margin;
}

bool AIRSPACE_ALT::is_below  (const ALTITUDE_STATE& state,
                              const fixed margin) const {
  return get_altitude(state) <= state.NavAltitude + margin ||
    /* special case: GND is always "below" the aircraft, even if the
       aircraft's AGL altitude turns out to be negative due to terrain
       file inaccuracies */
    (Base == abAGL && !positive(AGL));
}

fixed AIRSPACE_ALT::get_altitude(const ALTITUDE_STATE& state) const {
  return (Base == abAGL) ?
         AGL + (state.NavAltitude - state.AltitudeAGL) : Altitude;
}
