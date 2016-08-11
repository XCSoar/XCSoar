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
#ifndef AIRCRAFT_HPP
#define AIRCRAFT_HPP

#include "Geo/GeoPoint.hpp"
#include "Geo/SpeedVector.hpp"
#include "Compiler.h"

#include <type_traits>

/**
 * @file
 * Structures containing basic aircraft data used for navigation.
 * Typically this will be updated only by the GPS devices or by
 * a simulator.
 *
 * Copies of the states are made in some TaskPoint types so this should be
 * kept fairly small.  If extra data is required to be saved by the GPS
 * device, then this structure itself should be contained within a larger one.
 */

/**
 * Structure for speed-related state data
 */
struct SpeedState 
{
  //############
  //   Speeds
  //############

  /**
   * Speed over ground in m/s
   * @see TrueAirspeed
   * @see IndicatedAirspeed
   */
  double ground_speed;

  /**
   * True air speed (m/s)
   * @see Speed
   * @see IndicatedAirspeed
   */
  double true_airspeed;

  void Reset() {
    ground_speed = true_airspeed = 0;
  }
};

/**
 * Structure for altitude-related state data
 */
struct AltitudeState 
{
  //##############
  //   Altitude
  //##############

  /** Altitude used for navigation (GPS or Baro) */
  double altitude;

  /** Fraction of working band height */
  double working_band_fraction;

  /** Altitude over terrain */
  double altitude_agl;

  void Reset();
};

/**
 * Structure for variometer data
 */
struct VarioState
{
  /**
   * Rate of change of total energy of aircraft (m/s, up positive)
   * @see VarioAvailable
   */
  double vario;

  /**
   * Vertical speed of air mass (m/s, up positive)
   * @see NettoVarioAvailable
   */
  double netto_vario;

  void Reset(){
    vario = netto_vario = 0;
  }
};

/**
 * Compound structure defining an aircraft state
 */
struct AircraftState: 
  public AltitudeState,
  public SpeedState,
  public VarioState
{
  //##########
  //   Time
  //##########

  /**
   * Global time (seconds after UTC midnight).  A negative value means
   * "no time available", e.g. if no GPS fix was obtained yet.
   */
  double time;

  //################
  //   Navigation
  //################

  /** location of aircraft */
  GeoPoint location;

  /** track angle in degrees true */
  Angle track;

  //##################
  //   Acceleration
  //##################

  /**
   * G-Load information of external device (if available)
   * or estimated (assuming balanced turn) 
   * @see AccelerationAvailable
   */
  double g_load;

  /** Wind speed, direction at aircraft */
  SpeedVector wind;

  bool flying;

  /**
   * Calculate predicted state in future.
   * Assumes aircraft will continue along current TrackBearing and Speed with
   * constant climb speed
   * @param in_time Time step for extrapolation (s)
   * @return Predicted aircraft state in in_time seconds
   */
  gcc_pure
  AircraftState GetPredictedState(double in_time) const;

  void Reset();
};

static_assert(std::is_trivial<AircraftState>::value, "type is not trivial");

#endif
