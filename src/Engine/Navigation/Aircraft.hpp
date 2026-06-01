// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Geo/GeoPoint.hpp"
#include "Geo/SpeedVector.hpp"
#include "Math/EnergyHeight.hpp"
#include "time/FloatDuration.hxx"
#include "time/Stamp.hpp"

#include <chrono>
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

  constexpr void Reset() noexcept {
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

  /**
   * Convertible energy height for glide/MacCready (see #Math/EnergyHeight.hpp):
   * navigation altitude plus \f$(v^2 - v_{bg}^2)/(2g)\f$ when airspeed and polar
   * are available — not the aircraft geometric altitude.
   */
  double total_energy_height;

  constexpr void Reset() noexcept {
    altitude = 0;
    working_band_fraction = 0;
    altitude_agl = 0;
    total_energy_height = 0;
  }
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

  constexpr void Reset() noexcept {
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
  TimeStamp time;

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

  constexpr bool HasTime() const noexcept {
    return time.IsDefined();
  }

  constexpr void ResetTime() noexcept {
    time = TimeStamp::Undefined();
  }

  /**
   * Calculate predicted state in future.
   * Assumes aircraft will continue along current TrackBearing and Speed with
   * constant climb speed
   * @param in_time Time step for extrapolation (s)
   * @return Predicted aircraft state in in_time seconds
   */
  [[gnu::pure]]
  AircraftState GetPredictedState(FloatDuration in_time) const noexcept;

  constexpr void Reset() noexcept {
    AltitudeState::Reset();
    SpeedState::Reset();
    VarioState::Reset();

    ResetTime();
    location.SetInvalid();
    track = Angle::Zero();
    g_load = 1;
    wind = SpeedVector::Zero();
    flying = false;
  }
};

/**
 * Convertible energy height for task/MacCready prediction when TAS and polar
 * are available; otherwise navigation #AircraftState::altitude.
 */
[[gnu::pure]]
inline double
GlideEnergyHeightForAircraft(const AircraftState &ac,
                             double best_glide_true_airspeed_m_s) noexcept
{
  if (ac.true_airspeed > 0. && best_glide_true_airspeed_m_s > 0.)
    return GlideConvertibleEnergyHeight(ac.altitude, ac.true_airspeed,
                                        best_glide_true_airspeed_m_s);

  return ac.altitude;
}

/**
 * Height input for MacCready / GlideState: convertible energy when available,
 * otherwise geometric #AircraftState::altitude.
 */
[[gnu::pure]]
inline double
GlideHeightForMacCready(const AircraftState &ac) noexcept
{
  return ac.total_energy_height > 0 ? ac.total_energy_height : ac.altitude;
}

static_assert(std::is_trivial<AircraftState>::value, "type is not trivial");
