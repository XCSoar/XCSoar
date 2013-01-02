/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Navigation/Aircraft.hpp"
#include "Geo/GeoVector.hpp"

AircraftState 
AircraftState::GetPredictedState(const fixed &in_time) const
{
  AircraftState state_next = *this;
  GeoVector vec(ground_speed * in_time, track);
  state_next.location = vec.EndPoint(location);
  state_next.altitude += vario * in_time;
  return state_next;
}

void
AircraftState::Reset()
{
  AltitudeState::Reset();

  g_load = fixed_one;
  wind = SpeedVector::Zero();
  flying = false;
}

void
AltitudeState::Reset()
{
  altitude = fixed_zero;
  working_band_fraction = fixed_zero;
  altitude_agl = fixed_zero;
}
