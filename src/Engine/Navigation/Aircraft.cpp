/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
  GeoVector vec(Speed * in_time, TrackBearing);
  state_next.Location = vec.end_point(Location);
  state_next.NavAltitude += Vario * in_time;
  return state_next;
}

AIRCRAFT_STATE::AIRCRAFT_STATE():
  ALTITUDE_STATE(),
  Gload(fixed_one)
{
}

ALTITUDE_STATE::ALTITUDE_STATE():
  working_band_fraction(fixed_one)
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
  //
  // @todo accuracy: make this more robust by making use of terrain height data
  // if available

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
