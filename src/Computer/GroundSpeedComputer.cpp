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

#include "GroundSpeedComputer.hpp"
#include "NMEA/Info.hpp"

void
GroundSpeedComputer::Compute(NMEAInfo &basic)
{
  if (basic.ground_speed_available ||
      !basic.time_available || !basic.location_available) {
    if (!basic.ground_speed_available)
      basic.ground_speed = 0;

    delta_time.Reset();
    last_location_available.Clear();
    return;
  }

  if (!last_location_available)
    delta_time.Update(basic.time, 0, 0);
  else if (basic.location_available.Modified(last_location_available)) {
    const auto dt = delta_time.Update(basic.time, 0.2, 5);
    if (dt > 0) {
      auto distance = basic.location.DistanceS(last_location);
      basic.ground_speed = distance / dt;
      basic.ground_speed_available = basic.location_available;
    }
  }

  last_location = basic.location;
  last_location_available = basic.location_available;
}
