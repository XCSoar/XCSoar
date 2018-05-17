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

#include "FlyingComputer.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Engine/Navigation/Aircraft.hpp"

void
FlyingComputer::Reset()
{
  delta_time.Reset();

  stationary_clock.Clear();
  moving_clock.Clear();
  climbing_clock.Clear();
  moving_since = -1;
  stationary_since = -1;
  climbing_altitude = 0;
  sinking_since = -1;
  powered_since = -1;
  unpowered_since = -1;
  last_ground_altitude = -1;
}

inline void
FlyingComputer::CheckRelease(FlyingState &state, double time,
                             const GeoPoint &location, double altitude)
{
  if (!state.flying || state.release_time >= 0 ||
      stationary_clock.IsDefined())
    return;

  if (sinking_since < 0) {
    sinking_since = time;
    sinking_location = location;
    sinking_altitude = altitude;
    return;
  }

  if (time < sinking_since || altitude >= sinking_altitude) {
    /* cancel release detection when the aircraft has been climbing
       more than it has lost recently */
    sinking_since = -1;
    return;
  }

  if (time - sinking_since >= 10) {
    /* assume release from tow if aircraft has not gained any height
       for 10 seconds; there will be false negatives if the glider
       enters a thermal right after releasing */
    state.release_time = sinking_since;
    state.release_location = sinking_location;
    state.far_location.SetInvalid();
    state.far_distance = -1;
  }
}

void
FlyingComputer::Check(FlyingState &state, double time)
{
  // Logic to detect takeoff and landing is as follows:
  //   detect takeoff when above threshold speed for 10 seconds
  //
  //   detect landing when below threshold speed for 30 seconds

  if (!state.flying) {
    // We are moving for 10sec now
    if (moving_clock >= 10) {
      // We certainly must be flying after 10sec movement
      assert(moving_since >= 0);

      state.flying = true;
      state.takeoff_time = moving_since;
      state.takeoff_location = moving_at;
      state.flight_time = 0;

      /* when a new flight starts, forget the old release and power-on/off time */
      state.release_time = -1;
      state.power_on_time = -1;
      state.power_off_time = -1;
      state.far_location.SetInvalid();
      state.far_distance = -1;
    }
  } else {
    // update time of flight
    state.flight_time = time - state.takeoff_time;

    // We are not moving anymore for 60sec now
    if (!moving_clock.IsDefined()) {
      // We are probably not flying anymore
      assert(stationary_since >= 0);

      state.flying = false;
      state.flight_time = stationary_since - state.takeoff_time;
      state.landing_time = stationary_since;
      state.landing_location = stationary_at;
    }
  }

  // If we are not certainly flying we are probably on the ground
  // To make sure that we are, wait for 10sec to make sure there
  // is no more movement
  state.on_ground = !state.flying && stationary_clock >= 10;
}

void
FlyingComputer::Moving(FlyingState &state, double time, double dt,
                       const GeoPoint &location)
{
  // Increase InFlight countdown for further evaluation
  moving_clock.Add(dt);

  if (moving_since < 0) {
    moving_since = time;
    moving_at = location;
  }

  // We are moving so we are certainly not on the ground
  stationary_clock.Clear();
  stationary_since = -1;

  // Update flying state
  Check(state, time);
}

void
FlyingComputer::Stationary(FlyingState &state, double time, double dt,
                           const GeoPoint &location)
{
  // Decrease InFlight countdown for further evaluation
  if (moving_clock.IsDefined()) {
    moving_clock.Subtract(dt);
    if (!moving_clock.IsDefined())
      moving_since = -1;
  }

  stationary_clock.Add(dt);

  if (stationary_since < 0) {
    stationary_since = time;
    stationary_at = location;
  }

  // Update flying state
  Check(state, time);
}

gcc_pure
static bool
CheckTakeOffSpeed(double takeoff_speed, const NMEAInfo &basic)
{
  const auto speed = basic.airspeed_available
    ? (basic.airspeed_real || basic.ground_speed >= takeoff_speed / 4
       ? std::max(basic.true_airspeed, basic.ground_speed)
       /* at low ground speeds and an (unreal) airspeed vector derived
          from the wind vector, take only half of the wind vector into
          account, to reduce the risk of false negative landing
          detections due to bogus wind vectors */
       : (basic.true_airspeed + basic.ground_speed) / 2)
    : basic.ground_speed;

  // Speed too high for being on the ground
  return speed >= takeoff_speed;
}

/**
 * After take-off has been detected, we check if the ground speed goes
 * below a certain threshold that indicates the aircraft has ceased
 * flying.  To avoid false positives while wave/ridge soaring, this
 * threshold is half of the given take-off speed.
 */
gcc_pure
static bool
CheckLandingSpeed(double takeoff_speed, const NMEAInfo &basic)
{
  return !CheckTakeOffSpeed(takeoff_speed / 2, basic);
}

gcc_pure
static bool
CheckAltitudeAGL(const DerivedInfo &calculated)
{
  return calculated.altitude_agl_valid && calculated.altitude_agl >= 300;
}

inline bool
FlyingComputer::CheckClimbing(double dt, double altitude)
{
  if (altitude > climbing_altitude + 0.1)
    climbing_clock.Add(dt);
  else
    climbing_clock.Subtract(dt);

  climbing_altitude = altitude;

  return climbing_clock >= dt + 1;
}

inline void
FlyingComputer::CheckPowered(double dt, const NMEAInfo &basic,
                             FlyingState &flying)
{
  if (basic.engine_noise_level > 500 &&
      powered_since < 0) {
    powered_since = basic.time;
    powered_at = basic.location;

    unpowered_since = -1;
    unpowered_at.SetInvalid();
  } else if (basic.engine_noise_level <= 350 &&
             unpowered_since < 0) {
    unpowered_since = basic.time;
    unpowered_at = basic.location;

    powered_since = -1;
    powered_at.SetInvalid();
  }

  if (powered_since >= 0 && unpowered_since < 0 && basic.time - powered_since >= 30) {
    flying.powered = true;
    flying.power_on_time = powered_since;
    flying.power_on_location = powered_at;
  } else if (unpowered_since >= 0 && basic.time - unpowered_since >= 30) {
    flying.powered = false;
    flying.power_off_time = unpowered_since;
    flying.power_off_location = unpowered_at;
  }
}

void
FlyingComputer::Compute(double takeoff_speed,
                        const NMEAInfo &basic,
                        const DerivedInfo &calculated,
                        FlyingState &flying)
{
  if (!basic.time_available || !basic.location_available)
    return;

  const auto dt = delta_time.Update(basic.time, 0.5, 20);
  if (dt < 0) {
    Reset();
    flying.Reset();
  }

  if (dt <= 0)
    return;

  const auto any_altitude = basic.GetAnyAltitude();

  if (!basic.airspeed_available && !calculated.altitude_agl_valid &&
      any_altitude.first && last_ground_altitude >= 0 &&
      any_altitude.second > last_ground_altitude + 250) {
    /* lower the threshold for "not moving" when the aircraft is high
       above the take-off airfield and there's no airspeed probe; this
       shall reduce the risk of false landing detection when flying in
       strong head wind (e.g. ridge or wave) */
    auto dh = any_altitude.second - last_ground_altitude;

    if (dh > 1000)
      takeoff_speed /= 4;
    else if (dh > 500)
      takeoff_speed /= 2;
    else
      takeoff_speed = takeoff_speed * 2 / 3;
  }

  if (CheckTakeOffSpeed(takeoff_speed, basic) ||
      CheckAltitudeAGL(calculated))
    Moving(flying, basic.time, dt, basic.location);
  else if (!flying.flying ||
           (CheckLandingSpeed(takeoff_speed, basic) &&
            (!any_altitude.first || !CheckClimbing(dt, any_altitude.second))))
    Stationary(flying, basic.time, dt, basic.location);

  if (basic.engine_noise_level_available)
    CheckPowered(dt, basic, flying);

  if (any_altitude.first) {
    if (flying.on_ground)
      last_ground_altitude = any_altitude.second;

    CheckRelease(flying, basic.time, basic.location, any_altitude.second);
  } else
    sinking_since = -1;

  if (flying.flying && flying.release_location.IsValid()) {
    auto distance = basic.location.Distance(flying.release_location);
    if (distance > flying.far_distance) {
      flying.far_location = basic.location;
      flying.far_distance = distance;
    }
  }
}

void
FlyingComputer::Compute(double takeoff_speed,
                        const AircraftState &state, double dt,
                        FlyingState &flying)
{
  if (state.time < 0)
    return;

  if (state.ground_speed > takeoff_speed)
    Moving(flying, state.time, dt, state.location);
  else
    Stationary(flying, state.time, dt, state.location);
}

void
FlyingComputer::Finish(FlyingState &flying, double time)
{
  if (flying.flying && stationary_clock >= 5)
    moving_clock.Clear();

  Check(flying, time);
}
