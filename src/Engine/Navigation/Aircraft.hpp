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
#ifndef AIRCRAFT_HPP
#define AIRCRAFT_HPP

#include "GeoPoint.hpp"
#include "SpeedVector.hpp"
#include "Compiler.h"

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
  fixed ground_speed; 

  /**
   * True air speed (m/s)
   * @see Speed
   * @see IndicatedAirspeed
   */
  fixed true_airspeed;

  /**
   * Indicated air speed (m/s)
   * @see Speed
   * @see TrueAirspeed
   * @see AirDensityRatio
   */
  fixed indicated_airspeed;
};

/**
 * Structure for altitude-related state data
 */
struct AltitudeState 
{
  AltitudeState();

  //##############
  //   Altitude
  //##############

  /** Altitude used for navigation (GPS or Baro) */
  fixed altitude;

  /** Fraction of working band height */
  fixed working_band_fraction;

  /** Altitude over terrain */
  fixed altitude_agl;

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
  fixed vario;

  /**
   * Vertical speed of air mass (m/s, up positive)
   * @see NettoVarioAvailable
   */
  fixed netto_vario;
};

/**
 * Structure for flying state (takeoff/landing)
 */
struct FlyingState
{
  /** True if airborne, False otherwise */
  bool flying;
  /** Detects when glider is on ground for several seconds */
  bool on_ground;

  /** Time of flight */
  fixed flight_time;
  /** Time of takeoff */
  fixed takeoff_time;

  /** Reset flying state as if never flown */
  void Reset();
};

static_assert(is_trivial_clang<FlyingState>::value, "type is not trivial");

/**
 * Compound structure defining an aircraft state
 */
struct AircraftState: 
  public AltitudeState,
  public SpeedState,
  public VarioState,
  public FlyingState
{
  AircraftState();

  //##########
  //   Time
  //##########

  fixed time; /**< global time (seconds UTC) */

  //################
  //   Navigation
  //################

  GeoPoint location; /**< location of aircraft */

  Angle track; /**< track angle in degrees true */

  //##################
  //   Acceleration
  //##################

  /**
   * G-Load information of external device (if available)
   * or estimated (assuming balanced turn) 
   * @see AccelerationAvailable
   */
  fixed g_load;

  SpeedVector wind; /**< Wind speed, direction at aircraft */

/**
 * Calculate predicted state in future.
 * Assumes aircraft will continue along current TrackBearing and Speed with
 * constant climb speed
 * @param in_time Time step for extrapolation (s)
 * @return Predicted aircraft state in in_time seconds
 */
  gcc_pure
  AircraftState GetPredictedState(const fixed& in_time) const;

  void Reset();
};

#endif
