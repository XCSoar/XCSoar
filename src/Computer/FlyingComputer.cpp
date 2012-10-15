/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Engine/GlideSolvers/GlidePolar.hpp"

void
FlyingComputer::Reset()
{
  delta_time.Reset();

  stationary_clock.Clear();
  moving_clock.Clear();
  moving_since = fixed_minus_one;
  stationary_since = fixed_minus_one;
  climbing_since = fixed_minus_one;
  sinking_since = fixed_minus_one;
}

void
FlyingComputer::CheckRelease(FlyingState &state, fixed time,
                             const GeoPoint &location, fixed altitude)
{
  if (!state.flying || !negative(state.release_time) ||
      stationary_clock.IsDefined())
    return;

  if (negative(sinking_since)) {
    sinking_since = time;
    sinking_location = location;
    sinking_altitude = altitude;
    return;
  }

  if (time < sinking_since || altitude >= sinking_altitude) {
    /* cancel release detection when the aircraft has been climbing
       more than it has lost recently */
    sinking_since = fixed_minus_one;
    return;
  }

  if (time - sinking_since >= fixed(10)) {
    /* assume release from tow if aircraft has not gained any height
       for 10 seconds; there will be false negatives if the glider
       enters a thermal right after releasing */
    state.release_time = sinking_since;
    state.release_location = sinking_location;
  }
}

void
FlyingComputer::Check(FlyingState &state, fixed time)
{
  // Logic to detect takeoff and landing is as follows:
  //   detect takeoff when above threshold speed for 10 seconds
  //
  //   detect landing when below threshold speed for 30 seconds

  if (!state.flying) {
    // We are moving for 10sec now
    if (moving_clock >= fixed_ten) {
      // We certainly must be flying after 10sec movement
      assert(!negative(moving_since));

      state.flying = true;
      state.takeoff_time = moving_since;
      state.takeoff_location = moving_at;
      state.flight_time = fixed_zero;

      /* when a new flight starts, forget the old release time */
      state.release_time = fixed_minus_one;
    }
  } else {
    // update time of flight
    state.flight_time = time - state.takeoff_time;

    // We are not moving anymore for 60sec now
    if (!moving_clock.IsDefined()) {
      // We are probably not flying anymore
      assert(!negative(stationary_since));

      state.flying = false;
      state.flight_time = stationary_since - state.takeoff_time;
      state.landing_time = stationary_since;
      state.landing_location = stationary_at;
    }
  }

  // If we are not certainly flying we are probably on the ground
  // To make sure that we are, wait for 10sec to make sure there
  // is no more movement
  state.on_ground = !state.flying && stationary_clock >= fixed(10);
}

void
FlyingComputer::Moving(FlyingState &state, fixed time, fixed dt,
                       const GeoPoint &location)
{
  // Increase InFlight countdown for further evaluation
  moving_clock.Add(dt);

  if (negative(moving_since)) {
    moving_since = time;
    moving_at = location;
  }

  // We are moving so we are certainly not on the ground
  stationary_clock.Clear();
  stationary_since = fixed_minus_one;

  // Update flying state
  Check(state, time);
}

void
FlyingComputer::Stationary(FlyingState &state, fixed time, fixed dt,
                           const GeoPoint &location)
{
  // Decrease InFlight countdown for further evaluation
  if (moving_clock.IsDefined()) {
    moving_clock.Subtract(dt);
    if (!moving_clock.IsDefined())
      moving_since = fixed_minus_one;
  }

  stationary_clock.Add(dt);

  if (negative(stationary_since)) {
    stationary_since = time;
    stationary_at = location;
  }

  // Update flying state
  Check(state, time);
}

gcc_pure
static bool
CheckTakeOffSpeed(fixed takeoff_speed, const NMEAInfo &basic)
{
  const fixed speed = basic.airspeed_available
    ? std::max(basic.true_airspeed, basic.ground_speed)
    : basic.ground_speed;

  // Speed too high for being on the ground
  return speed >= takeoff_speed;
}

gcc_pure
static bool
CheckAltitudeAGL(const DerivedInfo &calculated)
{
  return calculated.altitude_agl_valid && calculated.altitude_agl >= fixed(300);
}

bool
FlyingComputer::CheckClimbing(fixed time, fixed altitude)
{
  if (negative(climbing_since) || altitude <= climbing_altitude) {
    climbing_since = time;
    climbing_altitude = altitude;
    return false;
  } else {
    climbing_altitude = altitude;
    return time >= climbing_since + fixed_ten;
  }
}

void
FlyingComputer::Compute(fixed takeoff_speed,
                        const NMEAInfo &basic,
                        const DerivedInfo &calculated,
                        FlyingState &flying)
{
  if (!basic.time_available || !basic.location_available)
    return;

  const fixed dt = delta_time.Update(basic.time, fixed_half, fixed(20));
  if (negative(dt)) {
    Reset();
    flying.Reset();
  }

  if (!positive(dt))
    return;

  const auto any_altitude = basic.GetAnyAltitude();

  if (CheckTakeOffSpeed(takeoff_speed, basic) ||
      (any_altitude.first && CheckClimbing(basic.time, any_altitude.second)) ||
      CheckAltitudeAGL(calculated))
    Moving(flying, basic.time, dt, basic.location);
  else
    Stationary(flying, basic.time, dt, basic.location);

  if (any_altitude.first)
    CheckRelease(flying, basic.time, basic.location, any_altitude.second);
  else
    sinking_since = fixed_minus_one;
}

void
FlyingComputer::Compute(fixed takeoff_speed,
                        const AircraftState &state, fixed dt,
                        FlyingState &flying)
{
  if (state.ground_speed > takeoff_speed)
    Moving(flying, state.time, dt, state.location);
  else
    Stationary(flying, state.time, dt, state.location);
}

void
FlyingComputer::Finish(FlyingState &flying, fixed time)
{
  if (flying.flying && stationary_clock >= fixed(5))
    moving_clock.Clear();

  Check(flying, time);
}
