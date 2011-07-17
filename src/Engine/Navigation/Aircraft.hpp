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
struct SPEED_STATE 
{
  //############
  //   Speeds
  //############

  /**
   * Speed over ground in m/s
   * @see TrueAirspeed
   * @see IndicatedAirspeed
   */
  fixed Speed; 

  /**
   * True air speed (m/s)
   * @see Speed
   * @see IndicatedAirspeed
   */
  fixed TrueAirspeed;

  /**
   * Indicated air speed (m/s)
   * @see Speed
   * @see TrueAirspeed
   * @see AirDensityRatio
   */
  fixed IndicatedAirspeed;
};

/**
 * Structure for altitude-related state data
 */
struct ALTITUDE_STATE 
{
  ALTITUDE_STATE();

  //##############
  //   Altitude
  //##############

  /** Altitude used for navigation (GPS or Baro) */
  fixed NavAltitude;

  /** Fraction of working band height */
  fixed working_band_fraction;

  /** Altitude over terrain */
  fixed AltitudeAGL;

  /** Thermal drift factor: 1 indicates drift rate equal to wind speed,
   0 indicates no drift. */
  gcc_pure
  fixed thermal_drift_factor() const;
};

/**
 * Structure for variometer data
 */
struct VARIO_STATE
{
  //###########
  //   Vario
  //###########

  /**
   * Rate of change of total energy of aircraft (m/s, up positive)
   * @see VarioAvailable
   */
  fixed Vario;

  /**
   * Vertical speed of air mass (m/s, up positive)
   * @see NettoVarioAvailable
   */
  fixed NettoVario;
};

/**
 * Structure for flying state (takeoff/landing)
 */
struct FLYING_STATE
{
  FLYING_STATE();

  /** True if airborne, False otherwise */
  bool   Flying;
  /** Detects when glider is on ground for several seconds */
  bool   OnGround;

  /** Time of flight */
  fixed FlightTime;
  /** Time of takeoff */
  fixed TakeOffTime;

  /**
   * Reset flying state as if never flown
   */
  void flying_state_reset();

  /**
   * Update flying state when moving 
   *
   * @param time Time the aircraft is moving
   */
  void flying_state_moving(const fixed time);

  /**
   * Update flying state when stationary 
   *
   * @param time Time the aircraft is stationary
   * @param on_ground Whether the aircraft is known to be on the ground
   */
  void flying_state_stationary(const fixed time);

private:
  void flying_state_check(const fixed time);
  int  TimeOnGround;
  int  TimeInFlight;
};

/**
 * Compound structure defining an aircraft state
 */
struct AIRCRAFT_STATE: 
  public ALTITUDE_STATE,
  public SPEED_STATE,
  public VARIO_STATE,
  public FLYING_STATE
{
  AIRCRAFT_STATE();

  //##########
  //   Time
  //##########

  fixed Time; /**< global time (seconds UTC) */

  //################
  //   Navigation
  //################

  GeoPoint Location; /**< location of aircraft */

  Angle track; /**< track angle in degrees true */

  //##################
  //   Acceleration
  //##################

  /**
   * G-Load information of external device (if available)
   * or estimated (assuming balanced turn) 
   * @see AccelerationAvailable
   */
  fixed Gload;

  /** Accessor for the aircraft location */
  const GeoPoint& get_location() const {
    return Location;
  }

  SpeedVector wind; /**< Wind speed, direction at aircraft */

/**
 * Calculate predicted state in future.
 * Assumes aircraft will continue along current TrackBearing and Speed with
 * constant climb speed
 * @param in_time Time step for extrapolation (s)
 * @return Predicted aircraft state in in_time seconds
 */
  gcc_pure
  AIRCRAFT_STATE get_predicted_state(const fixed& in_time) const;
};

#endif
