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

#include "Navigation/Aircraft.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

AIRCRAFT_STATE 
AIRCRAFT_STATE::get_predicted_state(const fixed &in_time) const
{
  AIRCRAFT_STATE state_next = *this;
  GeoVector vec(Speed * in_time, track);
  state_next.Location = vec.end_point(Location);
  state_next.NavAltitude += Vario * in_time;
  state_next.AirspaceAltitude += Vario * in_time;
  return state_next;
}

AIRCRAFT_STATE::AIRCRAFT_STATE():
  ALTITUDE_STATE(),
  Gload(fixed_one)
{
}

ALTITUDE_STATE::ALTITUDE_STATE():
  NavAltitude(fixed_zero),
  AirspaceAltitude(fixed_zero),
  working_band_fraction(fixed_one),
  AltitudeAGL(fixed_zero)
{
}

fixed 
ALTITUDE_STATE::thermal_drift_factor() const
{
  return sigmoid(AltitudeAGL / 100);
}

FLYING_STATE::FLYING_STATE()
{
  flying_state_reset();
}

void
FLYING_STATE::flying_state_reset()
{
  TimeInFlight = 0;
  TimeOnGround = 0;
  Flying = false;
  OnGround = false;
}

void
FLYING_STATE::flying_state_moving(const fixed time)
{
  // Increase InFlight countdown for further evaluation
  if (TimeInFlight < 60)
    TimeInFlight++;

  // We are moving so we are certainly not on the ground
  TimeOnGround = 0;

  // Update flying state
  flying_state_check(time);
}

void
FLYING_STATE::flying_state_stationary(const fixed time)
{
  // Decrease InFlight countdown for further evaluation
  if (TimeInFlight)
    TimeInFlight--;

  if (TimeOnGround<30)
    TimeOnGround++;

  // Update flying state
  flying_state_check(time);
}

void
FLYING_STATE::flying_state_check(const fixed time)
{
  // Logic to detect takeoff and landing is as follows:
  //   detect takeoff when above threshold speed for 10 seconds
  //
  //   detect landing when below threshold speed for 30 seconds

  if (!Flying) {
    // We are moving for 10sec now
    if (TimeInFlight > 10) {
      // We certainly must be flying after 10sec movement
      Flying = true;
      TakeOffTime = time;
      FlightTime = fixed_zero;
    }
  } else {
    // update time of flight
    FlightTime = time - TakeOffTime;

    // We are not moving anymore for 60sec now
    if (TimeInFlight == 0)
      // We are probably not flying anymore
      Flying = false;
  }

  // If we are not certainly flying we are probably on the ground
  // To make sure that we are, wait for 10sec to make sure there
  // is no more movement
  OnGround = (!Flying) && (TimeOnGround > 10);
}
