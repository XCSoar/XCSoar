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

#include "FlyingComputer.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"

void
FlyingComputer::Compute(const GlidePolar &glide_polar,
                        const NMEAInfo &basic, const NMEAInfo &last_basic,
                        const DerivedInfo &calculated,
                        FlyingState &flying)
{
  if (basic.HasTimeRetreatedSince(last_basic))
    flying.Reset();

  // GPS not lost
  if (!basic.location_available)
    return;

  // Speed too high for being on the ground
  const fixed speed = basic.airspeed_available
    ? std::max(basic.true_airspeed, basic.ground_speed)
    : basic.ground_speed;

  if (speed > glide_polar.GetVTakeoff() ||
      (calculated.altitude_agl_valid && calculated.altitude_agl > fixed(300)))
    flying.Moving(basic.time);
  else
    flying.Stationary(basic.time);
}
